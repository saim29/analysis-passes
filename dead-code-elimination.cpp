#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "dataflow.h"

using namespace llvm;

namespace {

  BitVector transfer_function(BitVector out, BitVector use, BitVector def) {

    return BitVector();
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

    void populate_lhs(Function &F) {

      for (BasicBlock &B : F) {

        int size = bvec_mapping.size();
        BitVector b(size, false);
        lhs.insert({&B, b});

        for (Instruction &I : B) {

          if (I.isUsedInBasicBlock(&B) || I.isUsedOutsideOfBlock(&B)) {

            unsigned ind = bvec_mapping[&I];
            lhs[&B][ind] = 1;

          }           
        }
      }    
    }

    void populate_rhs(Function &F) {

      for (BasicBlock &B : F) {

        int size = bvec_mapping.size();
        BitVector b(size, false);
        lhs.insert({&B, b});

        for (Instruction &I : B) {

          if (I.isUsedInBasicBlock(&B) || I.isUsedOutsideOfBlock(&B)) {

            //do not process function calls
            if (isa<CallInst>(&I))
              continue;

            // go over instruction args
            for (User::op_iterator op = I.op_begin(), opE = I.op_end(); op != opE; ++op) {

              Value* val = *op;
                
              unsigned ind = bvec_mapping[val];
              rhs[&B][ind] = 1;

            }
          }
        }
      }
    }

    void populate_use(Function &F){

      for (BasicBlock &B : F) {

        int size = bvec_mapping.size();
        BitVector b(size, false);
        lhs.insert({&B, b});

        for (Instruction &I : B) {

          if (!I.isUsedInBasicBlock(&B) && !I.isUsedOutsideOfBlock(&B) || isa<CallInst>(&I)) {

            // go over instruction args
            for (User::op_iterator op = I.op_begin(), opE = I.op_end(); op != opE; ++op) {

              Value* val = *op;
                
              unsigned ind = bvec_mapping[val];
              use[&B][ind] = 1;

            }
          }
        }
      }

    }


    virtual bool runOnFunction(Function& F) {

      map_indexes(F);
      populate_lhs(F);
      populate_rhs(F);
      populate_use(F);

      // initialize top element and bottom element according to the meetOp
      unsigned size_bitvec = bvec_mapping.size();

      //initialize data flow framework
      DFF dff(&F, false, INTERSECTION, size_bitvec, &transfer_function, false);

      // compute use and def sets here
      populate_gen_kill(F);


      return false;
    }

    virtual void getAnalysisUsage(AnalysisUsage& AU) const {
      AU.setPreservesAll();
    }

    void populate_gen_kill(Function &F) {
      int size = bvec_mapping.size();

      for (BasicBlock &B : F) {

        BitVector bvec(size, false);

       

        unsigned ind = bvec_mapping[&B];


      }

    }
  private:

    // sets
    VMap bvec_mapping;
    BBVal lhs;
    BBVal rhs;
    BBVal use;

  };

  char dce::ID = 0;
  RegisterPass<dce> X("dce", "ECE 5984 dce");
}