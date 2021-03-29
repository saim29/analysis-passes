#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "dataflow.h"

using namespace llvm;

namespace {

  BitVector transfer_function(BitVector out, BitVector use, BitVector def) {

  }

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