#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "dataflow.h"

using namespace llvm;

namespace {

  BitVector transfer_function(BitVector out, BitVector use, BitVector def) {

    return set_union(out, use);

  }

  class dominators : public FunctionPass {

  public:
    static char ID;
    dominators() : FunctionPass(ID) { }

    

    virtual bool runOnFunction(Function& F) {

      map_indexes(F);

      // initialize top element and bottom element according to the meetOp
      unsigned size_bitvec = BBMapping.size();

      //initialize data flow framework
      DFF dff(&F, false, INTERSECTION,  size_bitvec, &transfer_function, true);

      // compute use and def sets here
      populate_add_and_sub(F);

      // pass the use and def sets to the DFF
      dff.setGen(add);
      dff.setKill(sub);

      // pass everything to the dff and start the analysis
      dff.runAnalysis();

      // copy results from DFF to this pass for future use
      in = dff.getIN();
      out = dff.getOUT();

      // print the results
      dff.printRes<Value*>(BBMapping, "NULL", "NULL");      

      return false;
    }

    virtual void getAnalysisUsage(AnalysisUsage& AU) const {
      AU.setPreservesAll();
    }

    void map_indexes(Function &F) {

      unsigned ind = 0;

      for (BasicBlock &B: F) {

        if (BBMapping.find(&B) == BBMapping.end()) {
          BBMapping.insert({&B, ind});
        }

        ind++;

      }
    }

    void populate_add_and_sub(Function &F) {

      int size = BBMapping.size();
      
      for (BasicBlock &B : F) {

        BitVector bvec = BitVector(0, false);

        sub.insert({&B, bvec});

        unsigned ind = bvec[BBMapping[&B]];

        bvec[ind] = 1;
        add.insert({&B, bvec});

      }

    }

    bool dominates(BasicBlock *b1, BasicBlock *b2) {

      //check if b1 dominates b2

      unsigned ind1 = BBMapping[b1];
      unsigned ind2 = BBMapping[b2];

      if (out[b2][ind1] == true)
        return true;
      else
        return false;

    }

    void getImmediateDominators() {



    }

  private:

    VMap BBMapping;
    BBVal add;
    BBVal sub;

    BBVal in;
    BBVal out;

    std::map<BasicBlock*,BasicBlock*> immediateDominators;

  };

  char dominators::ID = 0;
  RegisterPass<dominators> X("dominators", "ECE 5984 dominators");
}
