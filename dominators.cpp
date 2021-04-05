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
    std::map<BasicBlock*,BasicBlock*> immediateDominators;

    dominators() : FunctionPass(ID) { }

    

    virtual bool runOnFunction(Function& F) {

      map_indexes(F);

      // initialize top element and bottom element according to the meetOp
      unsigned size_bitvec = BBMapping.size();

      //initialize data flow framework
      DFF dff(&F, false, INTERSECTION, size_bitvec, &transfer_function, false, SEPARABLE, NULL, NULL);

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

      // get immediate dominators for every node
      findImmediateDominators(F);

      // print the results
      dff.printRes<Value*>(BBMapping, "ADD", "SUB");

      return false;
    }

    virtual void getAnalysisUsage(AnalysisUsage& AU) const {
      AU.setPreservesAll();
    }

    void map_indexes(Function &F) {

      unsigned ind = 0;

      for (BasicBlock &B: F) {

          BBMapping.insert({&B, ind});
          ind = ind + 1;

      }
    }

    void populate_add_and_sub(Function &F) {

      int size = BBMapping.size();

      for (BasicBlock &B : F) {

        BitVector bvec(size, false);

        sub.insert({&B, bvec});

        unsigned ind = BBMapping[&B];

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

    bool dominates(Instruction *i1, Instruction *i2) {

      // check if i1 dominates i2

      BasicBlock *b1 = i1->getParent();
      BasicBlock *b2 = i2->getParent();

      if (b1 == b2) {

        return i1->comesBefore(i2);

      } else {

        return dominates(b1, b2);
      }

    }

    void findImmediateDominators(Function &F) {

      BasicBlock *entry = &F.getEntryBlock();
      for (BasicBlock &B :F) {

        immediateDominators[&B] = entry;

        std::queue<BasicBlock*> bfs;
        std::set<BasicBlock*> visited;

        bfs.push(&B);
        while (!bfs.empty()) {

          BasicBlock* curr = bfs.front();
          bfs.pop();

          visited.insert(curr);

          for (BasicBlock *pred : predecessors(curr)) {

            if (dominates(pred, &B) && dominates(immediateDominators[&B], pred) && curr != &B) {

              immediateDominators[&B] = pred;

            }

            if (visited.find(pred) == visited.end()) {

              bfs.push(pred);

            }

          }

        }

      }

    }

  private:

    VMap BBMapping;
    BBVal add;
    BBVal sub;

    BBVal in;
    BBVal out;

  };

  char dominators::ID = 0;
  static RegisterPass<dominators> Y("dominators", "ECE 5984 dominators");

}
