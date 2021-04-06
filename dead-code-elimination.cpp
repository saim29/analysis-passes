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

#include "dataflow.h"

using namespace llvm;

namespace {

  BitVector transfer_function(BitVector out, BitVector gen, BitVector kill) {

    // Basic transfer function equaton here
    BitVector intermediate = set_diff(out, kill);
    return set_union(intermediate, gen);
    
    
  }

  BitVector updateDepGen(BasicBlock *B, BitVector gen, BitVector out, 
    BBVal lhs, BBVal rhs, BBVal use, VMap bmap) {

    // BitVector this_gen(gen.size(), false);

    // for (unsigned i=0; i<gen.size(); i++) {

    //   if (use[B][i])
    //     this_gen[i] = 0;
    //   else
    //     this_gen[i] = gen[i];

    // }
    return gen;
  }

  BitVector updateDepKill(BasicBlock *B, BitVector kill, BitVector out, 
    BBVal lhs, BBVal rhs, BBVal use, VMap bmap) {
    
    BitVector this_lhs = lhs[B];
    BitVector this_rhs = rhs[B];
    unsigned size  = this_rhs.size();

    Value* rev_mapping[bmap.size()];

    for (auto ele : bmap) {

      unsigned ind = ele.second;
      Value* val = ele.first;

      rev_mapping[ind] = val;

    }


    for(int i=0; i< size; i++) {

      if (this_lhs[i] == 1 && out[i] == 0) {

        Instruction *I = dyn_cast<Instruction>(rev_mapping[i]);
        // go over instruction args
        for (User::op_iterator op = I->op_begin(), opE = I->op_end(); op != opE; ++op) {

          Value* val = *op;

          unsigned ind = bmap[val];
          kill[i] = 1;
          
        }
        // // update rhs of the specific var
        // if (this_rhs[i] == 1) {
        //   kill[i] = 1;
        // }

      }
    }
    return kill;
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

    BitVector populate_lhs(BasicBlock *B) {

      int size = bvec_mapping.size();
      BitVector b(size, false);
      

      for (Instruction &I : *B) {

          StringRef name = getShortValueName(&I);
          if (name[0] == '%') {
            
            unsigned ind = bvec_mapping[&I];
            b[ind] = 1;

          }
      } 
      return b;
    }

    BitVector populate_rhs(BasicBlock *B) {



      int size = bvec_mapping.size();
      BitVector b(size, false);


      for (Instruction &I : *B) {

          StringRef name = getShortValueName(&I);
          if (name[0] == '%') {

            //do not process function calls or instructions that may be assignments
            if (isa<CallInst>(&I))
              continue;


            // go over instruction args
            for (User::op_iterator op = I.op_begin(), opE = I.op_end(); op != opE; ++op) {

              Value* val = *op;
              
              // only map arguments that are coming from potential definitions
              // ignore any other arguments i.e constants etc
              if (Instruction *inst = dyn_cast<Instruction>(val)) {

                unsigned ind = bvec_mapping[inst];
                b[ind] = 1;
              }
            }
          }
      }
      return b;
    }

    BitVector populate_use(BasicBlock *B){

      int size = bvec_mapping.size();
      BitVector b(size, false);


      for (Instruction &I : *B) {

        if (getShortValueName(&I)[0] != '%' || isa<CallInst>(&I)) {

          // do not process branchInsts
          // if (I.isTerminator())
          //   continue;

          // go over instruction args
          for (User::op_iterator op = I.op_begin(), opE = I.op_end(); op != opE; ++op) {

            Value* val = *op;
              
            // only map arguments if they are definitions otherwise ignore
            if (Instruction *inst = dyn_cast<Instruction>(val)) {

              unsigned ind = bvec_mapping[inst];
              b[ind] = 1;
            }

          }
        }
      }

      return b;
    }

    virtual bool runOnFunction(Function& F) {

      map_indexes(F);
      

      // initialize top element and bottom element according to the meetOp
      unsigned size_bitvec = bvec_mapping.size();
      
      separability sep = NON_SEPARABLE;

      //initialize data flow framework
      DFF dff(&F, true, INTERSECTION, size_bitvec, &transfer_function, true, sep, &updateDepGen, &updateDepKill);

      // compute use and def sets here
      populate_gen_kill(F);

      // pass the use and def sets to the DFF
      dff.setGen(gen);
      dff.setKill(kill);

      if(sep == NON_SEPARABLE) {
        dff.setLhs(glob_lhs);
        dff.setRhs(glob_rhs);
        dff.setUse(glob_use);
        dff.set_bvec_mapping(bvec_mapping);
      }

      // pass everything to the dff and start the analysis
      dff.runAnalysis();

      // copy results from DFF to this pass for future use
      in = dff.getIN();
      out = dff.getOUT();

      // print results for debugging
      print_faint_vals(F);

      Value* rev_mapping[bvec_mapping.size()];

      for (auto ele : bvec_mapping) {

        unsigned ind = ele.second;
        Value* val = ele.first;

        rev_mapping[ind] = val;

      }

      for (BasicBlock &B : F) {

        for (Instruction &I : B) {

          if (!isLive(&I)) {
        
            //I.dump();
            
          }

        }

      }

      return false;
    }

    virtual void getAnalysisUsage(AnalysisUsage& AU) const {
      AU.setPreservesAll();
    }

    void populate_gen_kill(Function &F) {
      int size = bvec_mapping.size();

      for (BasicBlock &B : F) {

        // Lets go through each instruction. 
        // After everything is done, whatever is left is the relevant gen and kill for that block

        BitVector genSet(size, false);

        BitVector lhs = populate_lhs(&B);
        BitVector rhs = populate_rhs(&B);
        BitVector use = populate_use(&B);

        // ?
        glob_lhs.insert({&B, lhs});
        glob_rhs.insert({&B, rhs});
        glob_use.insert({&B, use});


        BitVector comp_rhs = rhs.flip();
        genSet = set_intersection(lhs, comp_rhs);

        //added for use
        // BitVector comp_use = use.flip();
        // genSet = set_intersection(genSet, comp_use);

        // This is constant kill. Dependent kill will be updated during actual analysis
        BitVector killSet(glob_use[&B]);

        gen.insert({&B, genSet});
        kill.insert({&B, killSet});

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

        StringRef bName = B.getName();

        outs () << "==============" + bName + "==============" << "\n";

        outs () << "\nIN: \n";
        print_val(in[&B], rev_mapping);

        outs () << "\n" << "gen" << "\n";
        print_val(gen[&B], rev_mapping);

        outs () << "\n" << "kill" << "\n";
        print_val(kill[&B], rev_mapping);

        outs () << "\nOUT: \n";
        print_val(out[&B], rev_mapping);

        outs () << "\nLHS: \n";
        print_val(glob_lhs[&B], rev_mapping);

        outs () << "\nRHS: \n";
        print_val(glob_rhs[&B], rev_mapping);

        outs () << "\nUSE: \n";
        print_val(glob_use[&B], rev_mapping);

        outs () << "\n====================================" << "\n";

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
    BBVal in;
    BBVal out;


    BBVal glob_lhs;
    BBVal glob_rhs;
    BBVal glob_use;
    BBVal gen;
    BBVal kill;

  };

  char dce::ID = 0;
  RegisterPass<dce> X("DCE", "ECE 5984 dce");
}