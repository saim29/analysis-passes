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
#include "dominators.cpp"

using namespace llvm;

namespace {

  class LICM : public LoopPass {

  public:
    static char ID;
    LICM() : LoopPass(ID) { }

    virtual void getAnalysisUsage(AnalysisUsage& AU) const {

      AU.addRequired<dominators>();
      AU.addRequired<LoopInfoWrapperPass>();

    }

    virtual bool runOnLoop(Loop *L, LPPassManager &LPM) {

      LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
      domTree = &getAnalysis<dominators>();

      // we only care about loops which have a distinct preHeader
      if (L->getLoopPreheader() != NULL) {

        bool modified = false;

        //print loop dominance info
        for (auto block : L->getBlocksVector()) {

          BasicBlock *imDom = domTree->immediateDominators[block];
          outs() << imDom->getName() << " dom " << block->getName() << "\n";

        }

        // set the landingPad to the loop preHeader initially
        BasicBlock *landingPad = L->getLoopPreheader();

        // we only want to insert a landing pad in a loop which is not in do; while form
        // do; while loops are automatically handled

        if (!L->isRotatedForm()) {

          //update landingPad to the newly created one if loop is not in  do; while form
          //landingPad = landingPadTransform(L);
          modified = true;
          
        }

        findLoopInvariantInsts(L);
        modified = codeMotion(L, landingPad);

        
        return modified;
      }


      return false;
    }



  private:

    // set for loop invariant instructions
    DenseSet<Value*> invariantInsts;
    LoopInfo *LI;
    dominators *domTree;


    // private methods

    BasicBlock* landingPadTransform(Loop *L) {

      BasicBlock *header = L->getHeader();
      BasicBlock *preHeader = L->getLoopPreheader();
      BasicBlock *exit = L->getExitBlock();
      BasicBlock *exiting = L->getExitingBlock();

      Instruction *header_branch = header->getTerminator();
      CmpInst *cond = dyn_cast<CmpInst>(header_branch->getOperand(0));

      // check for phi; if exits, get the split value
      Value *left = cond->getOperand(0);
      Value *right = cond->getOperand(1);

      if (auto *phi = dyn_cast<PHINode>(left)) {

        left = phi->getIncomingValueForBlock(preHeader);

      } 
      if (auto *phi = dyn_cast<PHINode>(right)) {

        right = phi->getIncomingValueForBlock(preHeader);

      }

      // split the preHeader block
      BasicBlock *landingPad = SplitBlock(preHeader, &preHeader->back());

      CmpInst *preHeader_branch = CmpInst::Create(cond->getOpcode(), cond->getPredicate(), left, right, Twine("cond"), preHeader->getTerminator());     

      // delete the previous terminator
      preHeader->getTerminator()->eraseFromParent();

      // add the new terminator
      BranchInst *preHeader_terminator = BranchInst::Create(landingPad, exit, preHeader_branch, preHeader);

      // get all phi instructions within the header block
      std::vector<Instruction*> phiList;
      for (BasicBlock::iterator it = BasicBlock::iterator(header->front()); it!=BasicBlock::iterator(header->getFirstNonPHI()); it++) {

        phiList.push_back(&*it);

      }

      Instruction *insertBefore = &exit->front();
      std::vector<Instruction*> newPhiList;
      for (auto phi : phiList) {

        PHINode *phi_node = dyn_cast<PHINode>(phi);

        Type *ty = phi->getType();
        const unsigned nVal = dyn_cast<PHINode>(phi)->getNumIncomingValues();
        PHINode *phi_new = PHINode::Create(ty, nVal, Twine("phi_new"), insertBefore);

        phi_new->addIncoming(phi_node->getIncomingValueForBlock(landingPad), preHeader);
        phi_new->addIncoming(phi_node, header);

        newPhiList.push_back(phi_new);
        //ReplaceInstWithValue(exit->getInstList(), phi_node, phi_new);
        //BasicBlock::iterator phi_itr = BasicBlock::iterator(phi_node);

        //ReplaceInstWithValue(exit->getInstList(), phi_itr, phi_new);

        // for (auto &U : phi->uses()) {
          
        // }

        //phi_node->replaceAllUsesWith(phi_new);
      }

      return landingPad;

    }

    bool isLoopInvariant(Instruction *I, Loop *L) {

      /*
        we do not need to compute reaching definitions because each variable has exactly one definition in SSA

        we use conditions given in the assignment to calculate invariant instructions plus we check if the 
        operands are loop invariant or not

      */

      // conditions for loop invariant
      if (isSafeToSpeculativelyExecute(I) && !I->mayReadFromMemory() && !isa<LandingPadInst>(I)) {

      // check operands if they are loop invariant, constant or from outside the loop
        for (User::op_iterator op = I->op_begin(), opE = I->op_end(); op!=opE; op++) {

          Value* val = *op;
          if (!dyn_cast<Constant>(val)){
            if (invariantInsts.find(val) == invariantInsts.end()) {
              if (Instruction *inst = dyn_cast<Instruction>(val))
                if (L->contains(inst))
                  return false;
            }
          }

        }

        return true;

      }

      return false;
      
    }

    void findLoopInvariantInsts(Loop *L) {

      // all basic blocks contained in the loop
      std::vector<BasicBlock*> blocks = L->getBlocksVector();

      //get current loop info
      Loop *cur_loop = LI->getLoopFor(L->getHeader());

      bool convergence = false;
      while (!convergence) {

        convergence = true;
        for(auto block : blocks) {

          if (LI->getLoopFor(block) != cur_loop) {

            // do not process subloops as they have already been processed (llvm LoopPass visits loop from the deepest loop)
            // we only need to process preHeaders of subloops which are part of our block vector by default

            continue;
          }

          // traverse all instructions in loop blocks
          for (Instruction &I : *block) {

            if ((invariantInsts.find(&I) == invariantInsts.end()) && isLoopInvariant(&I, L)) {
              invariantInsts.insert(&I);
              convergence = false;
            }

          }

        }

      }

    }

    bool codeMotion(Loop *L, BasicBlock *landingPad) {

      // move all instructions to the newly inserted landing pad
      /*
        move instructions only if 
        1) they are loop invariant (all insts in invariantInsts)
        2) assign to variable not assigned elsewhere in the loop (SSA handles this)
        3) in block that dominates loop exit (can figure out using our dom pass)
        4) all uses of instruction are dominated by the instruction (can get all uses of instruction and check if they are dominated by instruction)

      */

      // After loopSimplify, loop Header is both, entry and exit out of the loop hence we need to find the predecessors of the header inside the loop

      BasicBlock *header = L->getHeader();
      BasicBlock *preHeader = L->getLoopPreheader();

      // basic blocks must dominate all these blocks
      std::vector<BasicBlock*> loopExits;

      // set of instructions that can be moved to landingPad
      std::set<Instruction*> canBeMoved;

      for (BasicBlock *pred : predecessors(header)) {

        if (pred != preHeader) {

          loopExits.push_back(pred);

        }

      }

      for (auto inst : invariantInsts) {

        Instruction *i_inst = dyn_cast<Instruction>(inst);

        for (auto exit : loopExits) {

          BasicBlock *b_inst = i_inst->getParent();
          if (!domTree->dominates(b_inst, exit)) {

            continue;

          }

        }

        for (auto u : inst->users()) {
          
          if (auto u_inst = dyn_cast<Instruction>(u)) {
            if (!domTree->dominates(i_inst, u_inst)) {

              continue;

            }

          }

        }

        canBeMoved.insert(i_inst);

      }

      for (auto inst : canBeMoved) {

        // get the insertion point in the basic block
        Instruction *insertionPt = &preHeader->back();

        // insert the instruction
        inst->moveBefore(insertionPt);

      }

      if (canBeMoved.empty())
        return false;
      else
        return true;
    }

    static void replaceAllUsesAfter(Instruction *after, Value *toReplace, Value *toReplaceWith) {

      //traverse all instructions
      //traverse all BasicBlocks

      std::queue<BasicBlock> bfs;
      DenseSet<BasicBlock*> visited;

      BasicBlock *b = after->getParent();

      BasicBlock::iterator b_it= BasicBlock::iterator(after);

      for (auto it = b_it; it!=b->end(); it++) {

        //check for uses

      }

    }

  };

  char LICM::ID = 1;
  RegisterPass<LICM> Z("LICM", "ECE 5984 licm");
}