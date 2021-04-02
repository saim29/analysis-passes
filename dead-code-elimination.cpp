#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "dataflow.h"

using namespace llvm;

namespace {

  BitVector transfer_function(BitVector out, BitVector use, BitVector def) {

    return BitVector();
  }

  class faintAnalysis {

    public:
      faintAnalysis(Function &func) {

        map_indexes(func);
        populate_lhs(func);
        populate_rhs(func);
        populate_use(func);
        
      }

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

      // sets
      VMap bvec_mapping;
      BBVal lhs;
      BBVal rhs;
      BBVal use;

  };

  class dce : public FunctionPass {

  public:
    static char ID;
    dce() : FunctionPass(ID) { }

    

    virtual bool runOnFunction(Function& F) {

      return false;
    }

    virtual void getAnalysisUsage(AnalysisUsage& AU) const {
      AU.setPreservesAll();
    }


  };

  char dce::ID = 0;
  RegisterPass<dce> X("dce", "ECE 5984 dce");
}