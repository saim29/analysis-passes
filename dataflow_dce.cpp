
////////////////////////////////////////////////////////////////////////////////

#include "dataflow_dce.h"

  // Add code for your dataflow abstraction here.

  non_sep_dff::non_sep_dff() {

  }


  non_sep_dff::non_sep_dff(Function *F, bool direction, meetOperator meetOp, unsigned bitvec_size, transferFuncTy transferFunc,
      bool boundary_val, genKillUpdaterTy depGen, genKillUpdaterTy depKill) {
        
    this->F = F;
    this->direction = direction;
    this->meetOp = meetOp;
    this->transferFunc = transferFunc;
    this->T.resize(bitvec_size, false);
    this->B.resize(bitvec_size, false);
    this->updateDepGen = depGen;
    this->updateDepKill = depKill;
    
    // initialize top and bottom elements of the semi-lattice
    if (meetOp == INTERSECTION) {

      for(int i=0; i<bitvec_size; i++) {

        T[i] = 1;
      }

    } else if (meetOp == UNION) {

      for(int i=0; i<bitvec_size; i++) {

        B[i] = 1;

      }
    }

    // initial value of in and out 
    for (BasicBlock &B : *F) {

      for (Instruction &I: B) {
        if (direction) {
          in[&I] = T;
        } else {
          out[&I] = T;
        }
      }
    }

    // Boundary condition

    setBoundary(direction, boundary_val, bitvec_size);
  }

  non_sep_dff::~non_sep_dff() {

  }  

  IVal non_sep_dff::getIN() {

    return in;

  }

  IVal non_sep_dff::getOUT() {

    return out;

  }

  void non_sep_dff::setGen(IVal gen) {

    this->gen = gen;

  }

  void non_sep_dff::setKill(IVal kill) {

    this->kill = kill;

  }

  void non_sep_dff::setLhs(IVal glob_lhs) {

    this->glob_lhs = glob_lhs;

  }

  void non_sep_dff::setRhs(IVal glob_rhs) {

    this->glob_rhs = glob_rhs;

  }
  void non_sep_dff::setUse(IVal glob_use) {

    this->glob_use = glob_use;

  }

  void non_sep_dff::set_bvec_mapping(VMap mapping) {

    this->bvec_mapping = mapping;
  }

  void non_sep_dff::setBoundary(bool direction, bool boundary_val, unsigned bitvec_size) {

    BitVector init_val = BitVector(bitvec_size, boundary_val);

    if(direction == 0) { // Forwards

      BasicBlock &B = F->getEntryBlock();
      Instruction &I = B.front();
      out[&I] = init_val;
      out_entry = init_val;

    }

    else {  // Backwards

      for (auto ele: getPossibleExitInsts()) {
        in[ele] = init_val;
      }

      in_exit = init_val;
    }
  }

  IList non_sep_dff::getPossibleExitInsts() {

    IList ret;

    for (BasicBlock &B: *F) {

      for (Instruction &I: B) {

        if (dyn_cast<ReturnInst>(&I)) {
          ret.push_back(&I);
          break;
        }
      }
    }
    return ret;
  }

  std::vector<BasicBlock*> non_sep_dff::getPossibleExitBlocks() {

    std::vector<BasicBlock*> ret;

    for (BasicBlock &B: *F) {

      for (Instruction &I: B) {

        if (dyn_cast<ReturnInst>(&I)) {
          ret.push_back(&B);
          break;
        }
      }
    }
    return ret;
  }


  bool non_sep_dff::traverseBlockBackwards(BasicBlock *B) {

    bool changed = false;
    Instruction *prev = &B->back();

    for(BasicBlock::reverse_iterator it = B->rbegin(), bEnd = B->rend(); it != bEnd; ++it) {

      Instruction *inst = &*it;

      if (inst != prev) {

        out[inst] = in[prev];

      }

      BitVector old_in = in[inst];

      kill[inst] = updateDepKill(inst, kill[inst], out[inst], glob_lhs, glob_rhs, glob_use, bvec_mapping);
      gen[inst] = updateDepGen(inst, gen[inst], out[inst], glob_lhs, glob_rhs, glob_use, bvec_mapping);

      BitVector new_in = transferFunc(out[inst], gen[inst], kill[inst]);

      in[inst] = new_in;

      // compare the new in and out to the old ones
      if (new_in != old_in) 
        changed = true;

      prev = inst;
    }

    return changed;
  }

  BitVector non_sep_dff::applyMeet(BitVector b1, BitVector b2) {

    BitVector output;

    if (meetOp == INTERSECTION) {

      output = set_intersection_dce(b1, b2);

    } else if (meetOp == UNION) {

      output = set_union_dce(b1, b2);
    }
    return output;
  }

  void non_sep_dff::runAnalysis() {

    outs () << "********** Function: " + F->getName() + " ***********" << "\n";

    unsigned numIter = 0;

    if (direction) {

      // backward analysis
      std::vector<BasicBlock*> exitBlockPreds = getPossibleExitBlocks();
      bool changed = false;

      do {

        changed = false;
        std::queue<BasicBlock*> bfs;
        DenseSet<BasicBlock*> visited;
        
        for(auto ele : exitBlockPreds) {

          bfs.push(ele);

        }

        while (!bfs.empty()) {

          BasicBlock *curr = bfs.front();
          bfs.pop();

          visited.insert(curr);

          // apply meet operator here
          BitVector meetRes;
          for (BasicBlock *succ : successors(curr)) {

            meetRes = in[&succ->front()];
            break;

          }
          for (BasicBlock *succ : successors(curr)) {

            meetRes = applyMeet(meetRes, in[&succ->front()]);

          }

          // Calculate this iteration's out
          if (meetRes.size())
            out[&curr->back()] = meetRes;
          else
            out[&curr->back()] = in_exit;

          changed = traverseBlockBackwards(curr);

          // push all succcessors in the queue
          for (BasicBlock *pred : predecessors(curr)) {

            if (visited.find(pred) == visited.end()) {

              bfs.push(pred);

            }
          }

          // BitVector old_in = in[curr];
          // BitVector old_out = out[curr];

          // // call transfer function
          // BitVector new_in = transferFunc(out[curr], gen[curr], kill[curr]);
          // in[curr] = new_in;

          // // compare the new in and out to the old ones
          // if (new_in != old_in) 
          //   changed = true;

        }

        numIter++;
      } while (changed);

    }

    outs() << "Convergence: " << numIter << " iterations" <<"\n";
  }

  BitVector set_union_dce(BitVector b1, BitVector b2) {

    unsigned size = b1.size();
    BitVector u = BitVector(size);

    for (int i=0; i<size; i++) {

        u[i] = b1[i] || b2[i];
    }

    return u;
  }

  BitVector set_intersection_dce(BitVector b1, BitVector b2) {

    unsigned size = b1.size();
    BitVector u = BitVector(size);

    for (int i=0; i<size; i++) {

        u[i] = b1[i] && b2[i];
    }

    return u;

  }

  BitVector set_diff_dce(BitVector b1, BitVector b2) {

    unsigned size = b1.size();
    BitVector u = BitVector(size);

    for (int i=0; i<size; i++) {

        if (b2[i] == 1) {

          u[i] = 0;

        } else {

          u[i] = b1[i];

        }
    }

    return u;

  }