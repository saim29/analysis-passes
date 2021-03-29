#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "dataflow.h"

using namespace llvm;

namespace {

  BitVector transfer_function(BitVector out, BitVector use, BitVector def) {

  }

  class licm : public FunctionPass {

  public:
    static char ID;
    licm() : FunctionPass(ID) { }

    

    virtual bool runOnFunction(Function& F) {

      return false;
    }

    virtual void getAnalysisUsage(AnalysisUsage& AU) const {
      AU.setPreservesAll();
    }


  };

  char licm::ID = 0;
  RegisterPass<licm> X("licm", "ECE 5984 licm");
}