#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/IR/IntrinsicInst.h"
#include "llvm/ADT/PostOrderIterator.h"

#include "llvm/IR/Function.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/LegacyPassManagers.h"

#include "llvm/IR/InstrTypes.h"

#include "dataflow_dce.h"

using namespace llvm;

namespace {

  BitVector transfer_function(BitVector out, BitVector gen, BitVector kill) {

    // Basic transfer function equaton here
    BitVector intermediate = set_diff_dce(out, kill);
    return set_union_dce(intermediate, gen);
    
    
  }

  BitVector updateDepGen(Instruction *I, BitVector gen, BitVector out, 
    IVal lhs, IVal rhs, IVal use, VMap bmap) {

    // BitVector this_gen(gen.size(), false);

    // for (unsigned i=0; i<gen.size(); i++) {

    //   if (use[B][i])
    //     this_gen[i] = 0;
    //   else
    //     this_gen[i] = gen[i];

    // }
    return gen;

  }

  BitVector updateDepKill(Instruction *I, BitVector kill, BitVector out, 
    IVal lhs, IVal rhs, IVal use, VMap bmap) {
    
    BitVector this_lhs = lhs[I];
    BitVector this_rhs = rhs[I];
    BitVector this_kill = kill;
    BitVector this_out = out;
    BitVector this_use  = use[I]; 
    unsigned size  = this_rhs.size();

    //Value* rev_mapping[bmap.size()];

    //for (auto ele : bmap) {

    // unsigned ind = ele.second;
    // Value* val = ele.first;

    // rev_mapping[ind] = val;

    //}

    unsigned ind = bmap[I];

    if(this_out[ind] == 0 && this_lhs[ind] == 1) {

      // I->dump();

      // outs () << "killed\n";
      for (User::op_iterator op = I->op_begin(), opE = I->op_end(); op != opE; ++op) {

        Value* val = *op;

        if (isa<Instruction>(val)) {

          // val->dump();
          unsigned ind_op = bmap[val];
          this_kill[ind_op] = 1;
        }
      }
    }
    return this_kill;

  }

  class dce : public FunctionPass {

  public:
    static char ID;
    dce() : FunctionPass(ID) { }

    // used to determine i 
    bool isLive(Instruction *I) {

      if (I->isTerminator() || isa<DbgInfoIntrinsic>(I) || isa<LandingPadInst>(I) || I->mayHaveSideEffects()) {

          return true;
        }

      // if (I->getNumUses() > 0)
      //   return true;

      return false;
    }

    void map_indexes(Function &F) {

      unsigned ind = 0;

      for (BasicBlock &B : F) {

        for (Instruction &I : B) {

          StringRef name = getShortValueName(&I);

          if (name[0] == '%') {

            bvec_mapping.insert({&I, ind});
            ind++;

          }
        }
      }
    }

    BitVector populate_lhs(Instruction *I) {

      int size = bvec_mapping.size();
      BitVector b(size, false);

      StringRef name = getShortValueName(I);
      if (name[0] == '%') {
        
        unsigned ind = bvec_mapping[I];
        b[ind] = 1;

      }

      return b;
    }

    BitVector populate_rhs(Instruction *I) {

      int size = bvec_mapping.size();
      BitVector b(size, false);

      StringRef name = getShortValueName(I);
      if (name[0] == '%') {

        //do not process function calls or instructions that may be assignments
        if (isa<CallInst>(I))
          return b;


        // go over instruction args
        for (User::op_iterator op = I->op_begin(), opE = I->op_end(); op != opE; ++op) {

          Value* val = *op;
          
          // only map arguments that are coming from potential definitions
          // ignore any other arguments i.e constants etc
          if (Instruction *inst = dyn_cast<Instruction>(val)) {

            unsigned ind = bvec_mapping[inst];
            b[ind] = 1;
          }
        }
      }
      return b;
    }

    BitVector populate_use(Instruction *I){

      int size = bvec_mapping.size();
      BitVector b(size, false);

      if (getShortValueName(I)[0] != '%' || isa<CallInst>(I)) {

        // do not process branchInsts
        // if (I.isTerminator())
        //   continue;

        // go over instruction args
        for (User::op_iterator op = I->op_begin(), opE = I->op_end(); op != opE; ++op) {

          Value* val = *op;
            
          // only map arguments if they are definitions otherwise ignore
          if (Instruction *inst = dyn_cast<Instruction>(val)) {

            unsigned ind = bvec_mapping[inst];
            b[ind] = 1;
          }

        }
      }

      return b;
    }

    virtual bool runOnFunction(Function& F) {

      map_indexes(F);
      

      // initialize top element and bottom element according to the meetOp
      unsigned size_bitvec = bvec_mapping.size();
      
      //initialize data flow framework
      non_sep_dff dff(&F, true, INTERSECTION, size_bitvec, &transfer_function, true, &updateDepGen, &updateDepKill);

      // compute use and def sets here
      populate_gen_kill(F);

      // pass the use and def sets to the DFF
      dff.setGen(gen);
      dff.setKill(kill);

      dff.setLhs(glob_lhs);
      dff.setRhs(glob_rhs);
      dff.setUse(glob_use);
      dff.set_bvec_mapping(bvec_mapping);

      // pass everything to the dff and start the analysis
      dff.runAnalysis();

      // copy results from DFF to this pass for future use
      in = dff.getIN();
      out = dff.getOUT();

      // print results for debugging
      print_faint_vals(F);



      /*
        calculate instructions to delete
      */

      std::vector<Instruction*> toDel;

      // traverse in reverse post order
      for (po_iterator<BasicBlock*> BB = po_begin(&F.getEntryBlock()), BBEnd = po_end(&F.getEntryBlock()); BB != BBEnd; ++BB) {

        for (BasicBlock::reverse_iterator it = BB->rbegin(); it != BB->rend(); ++it) {

          Instruction *candidate = &*it;
          
          if (isLive(candidate))
            continue;

          // check if it is in out set
          BitVector candidate_out = out[candidate];
          unsigned ind = bvec_mapping[candidate];

          if (candidate_out[ind]) {
            toDel.push_back(candidate);
          }

        }

      }

      outs() << "\n\n--------- FAINT ---------------\n";
      // delete
      for(auto ele: toDel) {

        // don't delete instructions that have persistent uses
        if (ele->getNumUses() == 0) {

          outs () << "Deleting faint instruction : ";
          ele->dump();
          ele->eraseFromParent();

        }

      }

      outs() << "--------- FAINT END ---------------\n\n";


      return false;
    }

    virtual void getAnalysisUsage(AnalysisUsage& AU) const {
      AU.setPreservesAll();
    }

    void populate_gen_kill(Function &F) {
      int size = bvec_mapping.size();

      for (BasicBlock &B : F) {

        for (Instruction &I : B) {

          // Lets go through each instruction. 
          // After everything is done, whatever is left is the relevant gen and kill for that block

          BitVector genSet(size, false);

          BitVector lhs = populate_lhs(&I);
          BitVector rhs = populate_rhs(&I);
          BitVector use = populate_use(&I);

          // ?
          glob_lhs.insert({&I, lhs});
          glob_rhs.insert({&I, rhs});
          glob_use.insert({&I, use});


          BitVector comp_rhs = rhs.flip();
          genSet = set_intersection_dce(lhs, comp_rhs);

          //added for use
          // BitVector comp_use = use.flip();
          // genSet = set_intersection(genSet, comp_use);

          // This is constant kill. Dependent kill will be updated during actual analysis
          BitVector killSet(glob_use[&I]);

          gen.insert({&I, genSet});
          kill.insert({&I, killSet});
        }

      }

    }

    void print_faint_vals(Function &F) {


      Value* rev_mapping[bvec_mapping.size()];

      for (auto ele : bvec_mapping) {

        unsigned ind = ele.second;
        Value* val = ele.first;

        rev_mapping[ind] = val;

      }

      for (BasicBlock &B : F) {

        for (Instruction &I: B) {

          outs () << "============================" << "\n";
          I.dump();
          outs () << "============================" << "\n";

          outs () << "\nIN: \n";
          print_val(in[&I], rev_mapping);

          outs () << "\n" << "gen" << "\n";
          print_val(gen[&I], rev_mapping);

          outs () << "\n" << "kill" << "\n";
          print_val(kill[&I], rev_mapping);

          outs () << "\nOUT: \n";
          print_val(out[&I], rev_mapping);

          outs () << "\nLHS: \n";
          print_val(glob_lhs[&I], rev_mapping);

          outs () << "\nRHS: \n";
          print_val(glob_rhs[&I], rev_mapping);

          outs () << "\nUSE: \n";
          print_val(glob_use[&I], rev_mapping);

          outs () << "\n====================================" << "\n";

        }
      }
    }

    void print_val(BitVector b, Value *rev_mapping[]) {

      for (int i=0; i<b.size(); i++) {

        if (b[i]) {
          rev_mapping[i]->dump();
          // outs() << rev_mapping[i]->getName() << ",  ";
        }
      }

      outs () << "\n";

    }

  private:

    // sets
    VMap bvec_mapping;
    IVal in;
    IVal out;


    IVal glob_lhs;
    IVal glob_rhs;
    IVal glob_use;
    IVal gen;
    IVal kill;

  };

  char dce::ID = 0;
  RegisterPass<dce> X("DCE", "ECE 5984 dce");
}
