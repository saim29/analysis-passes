#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "dataflow.h"

using namespace llvm;

namespace {

  class CFGLoop {

    public:

    CFGLoop(BasicBlock *h, BasicBlock *e) {

      this->header = h;
      this->end = e;

      populate();

    }

    BasicBlock *header;
    BasicBlock *end;

    std::set<BasicBlock*> loopBody;

    void populate() {

      std::queue<BasicBlock*> bfs;
      std::set<BasicBlock*> visited;

      loopBody.insert(end);
      bfs.push(end);

      bool kill = false;
      while (!kill) {

        BasicBlock* curr = bfs.front();
        bfs.pop();
        visited.insert(curr);

        for (BasicBlock *pred : predecessors(curr)) {

          if (pred == header) {
            kill = true;
          }

          if (visited.find(pred) == visited.end()) {

            bfs.push(pred);
            loopBody.insert(pred);
          }
        }
      }
    }
  };

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

      // print the results
      dff.printRes<Value*>(BBMapping, "ADD", "SUB");

      std::vector<CFGLoop> loops = getCFGLoops(&F.getEntryBlock());
      getImmediateDominators(F);

      outs() << "Number of Loops Found: " << loops.size() << "\n";
      outs() << "Immediate Dominators For Loops: \n";
      for (auto loop : loops) {

        outs() << "\nLoop: " << loop.header->getName() << "\n";

        for (auto bb: loop.loopBody) {

          outs() << bb->getName() << ":   " << immediateDominators[bb]->getName() << "\n";


        }

      }

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

    std::vector<CFGLoop> getCFGLoops(BasicBlock* entry) {
      
      std::stack<BasicBlock*> dfs;
      std::set<BasicBlock*> visited;

      std::vector<CFGLoop> loops;

      dfs.push(entry);

      while(!dfs.empty()) {

        BasicBlock *curr = dfs.top();
        dfs.pop();
        visited.insert(curr);

        for (BasicBlock *succ : successors(curr)) {

          if (visited.find(succ) == visited.end()) {

            dfs.push(succ);

          } else {

            if (dominates(succ, curr)) {

              //loop header succ, loop end curr;
              CFGLoop loop(succ, curr);
              loops.push_back(loop);
            }

          }

        }
      }

      return loops;

    }

    void getImmediateDominators(Function &F) {

      BasicBlock *entry = &F.getEntryBlock();
      for (BasicBlock &B :F) {

        immediateDominators[&B] = entry;

        std::queue<BasicBlock*> bfs;
        std::set<BasicBlock*> visited;

        bfs.push(&B);
        while (!bfs.empty()) {

          BasicBlock* curr = bfs.front();
          bfs.pop();

          visited.insert(&B);

          for (BasicBlock *pred : predecessors(curr)) {

            if (dominates(pred, curr) && dominates(immediateDominators[curr], pred)) {

              immediateDominators[curr] = pred;

            }

            if (visited.find(curr) == visited.end()) {

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

    std::map<BasicBlock*,BasicBlock*> immediateDominators;

  };

  char dominators::ID = 0;
  RegisterPass<dominators> X("dominators", "ECE 5984 dominators");

}
