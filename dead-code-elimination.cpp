#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "dataflow.h"

using namespace llvm;

namespace {

  BitVector transfer_function(BitVector out, BitVector gen, BitVector kill) {

    // Basic transfer function equaton here
    BitVector intermediate = set_diff(out, kill);
    return set_union(intermediate, gen);
    
    
  }

  BitVector updateDepGen(BasicBlock *B, BitVector gen, BitVector out) {
    return gen;
  }

  BitVector updateDepKill(BasicBlock *B, BitVector gen, BitVector out) {
    return gen;
  }


  

  class dce : public FunctionPass {

  public:
    static char ID;
    dce() : FunctionPass(ID) { }

  

    void map_indexes(Function &F) {

      unsigned ind = 0;

      for (BasicBlock &B : F) {

        for (Instruction &I : B) {

          if (I.isUsedInBasicBlock(&B) || I.isUsedOutsideOfBlock(&B)) {

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
              
            unsigned ind = bvec_mapping[val];
            b[ind] = 1;

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
              
            unsigned ind = bvec_mapping[val];
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
      DFF dff(&F, true, INTERSECTION, size_bitvec, &transfer_function, true, NON_SEPARABLE, &updateDepGen, &updateDepKill);

      // compute use and def sets here
      populate_gen_kill(F);

      // pass the use and def sets to the DFF
      dff.setGen(gen);
      dff.setKill(kill);

      // pass everything to the dff and start the analysis
      dff.runAnalysis();

      // copy results from DFF to this pass for future use
      in = dff.getIN();
      out = dff.getOUT();



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

        BitVector comp_rhs = rhs.flip();
        genSet = set_intersection(lhs, comp_rhs);

        // This is constant kill. Dependent kill will be updated during actual analysis
        killSet = use;

        gen.insert({&B, genSet});
        kill.insert({&B, killSet});

      }

    }
  private:

    // sets
    VMap bvec_mapping;
    BBVal in;
    BBVal out;



    BBVal gen;
    BBVal kill;

  };

  char dce::ID = 0;
  RegisterPass<dce> X("dce", "ECE 5984 dce");
}