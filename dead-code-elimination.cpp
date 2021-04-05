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
    BBVal lhs, BBVal rhs, BBVal use) {
    return gen;
  }

  BitVector updateDepKill(BasicBlock *B, BitVector kill, BitVector out, 
    BBVal lhs, BBVal rhs, BBVal use) {
    
    BitVector this_rhs = rhs[B];
    unsigned size  = rhs.size();

    for(int i=0; i< size; i++) {
      if (this_rhs[i] == 1 && out[i] == 0) {
        kill[i] = 1;
      }
    }
    return kill;
  }


  

  class dce : public FunctionPass {

  public:
    static char ID;
    dce() : FunctionPass(ID) { }

  
    bool isLive(Instruction *I){

      if (I->isTerminator() || isa<DbgInfoIntrinsic>(I) || isa<LandingPadInst>(I), I->mayHaveSideEffects()) {

          return false;
        }

      if (I->getNumUses() > 0)
        return false;

      return true;
    }

    void map_indexes(Function &F) {

      unsigned ind = 0;

      for (BasicBlock &B : F) {

        for (Instruction &I : B) {

            bvec_mapping.insert({&I, ind});
            ind++;

        }
      }
    }

    BitVector populate_lhs(BasicBlock *B) {

      int size = bvec_mapping.size();
      BitVector b(size, false);
      

      for (Instruction &I : *B) {

        if (I.isUsedInBasicBlock(B) || I.isUsedOutsideOfBlock(B)) {

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

        if (I.isUsedInBasicBlock(B) || I.isUsedOutsideOfBlock(B)) {

          //do not process function calls
          if (isa<CallInst>(&I))
            continue;

          // go over instruction args
          for (User::op_iterator op = I.op_begin(), opE = I.op_end(); op != opE; ++op) {

            Value* val = *op;
            
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

        if (!I.isUsedInBasicBlock(B) && !I.isUsedOutsideOfBlock(B) || isa<CallInst>(&I)) {

          // go over instruction args
          for (User::op_iterator op = I.op_begin(), opE = I.op_end(); op != opE; ++op) {

            Value* val = *op;
              
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
      DFF dff(&F, true, INTERSECTION, size_bitvec, &transfer_function, false, sep, &updateDepGen, &updateDepKill);

      // compute use and def sets here
      populate_gen_kill(F);

      // pass the use and def sets to the DFF
      dff.setGen(gen);
      dff.setKill(kill);

      if(sep == NON_SEPARABLE) {
        dff.setLhs(glob_lhs);
        dff.setRhs(glob_rhs);
        dff.setUse(glob_use);
      }

      // pass everything to the dff and start the analysis
      dff.runAnalysis();

      // copy results from DFF to this pass for future use
      in = dff.getIN();
      out = dff.getOUT();

      // print results for debugging
      print_faint_vals(F);

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
        BitVector killSet(size, false);

        BitVector lhs = populate_lhs(&B);
        BitVector rhs = populate_rhs(&B);
        BitVector use = populate_use(&B);
        glob_lhs.insert({&B, genSet});
        glob_rhs.insert({&B, killSet});
        glob_use.insert({&B, genSet});


        BitVector comp_rhs = rhs.flip();
        genSet = set_intersection(lhs, comp_rhs);

        // This is constant kill. Dependent kill will be updated during actual analysis
        killSet = use;

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
    DenseMap<Instruction*,unsigned> bvec_mapping;
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