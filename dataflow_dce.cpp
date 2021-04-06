
////////////////////////////////////////////////////////////////////////////////

#include "dataflow.h"

namespace llvm {

  // Add code for your dataflow abstraction here.

  DFF::DFF() {

  }

  void DFF::setBoundary(bool direction, bool boundary_val, unsigned bitvec_size) {

    BitVector init_val = BitVector(bitvec_size, boundary_val);

    //added for dominators
    // init_val[0] = 1;

    if(direction == 0) { // Forwards
      out[&F->getEntryBlock()] = init_val;
      out_entry = init_val;
    }

    else {  // Backwards
      for (auto ele: getPossibleExitBlocks()) {
        in[ele] = init_val;
      }

      in_exit = init_val;
    }
  }

  DFF::DFF(Function *F, bool direction, meetOperator meetOp, unsigned bitvec_size, transferFuncTy transferFunc,
           bool boundary_val, separability sep, genKillUpdaterTy depGen, genKillUpdaterTy depKill) {

    this->F = F;
    this->direction = direction;
    this->meetOp = meetOp;
    this->transferFunc = transferFunc;
    this->T.resize(bitvec_size, false);
    this->B.resize(bitvec_size, false);
    this->sep = sep;
    this->updateDepGen = depGen;
    this->updateDepKill = depKill;
    
    // initialize top and bottom elements of the semi-lattice
    if (meetOp == INTERSECTION) {

      for(int i=0; i<bitvec_size; i++) {

        T[i] = 1;
        // B[i] = 0;

      }

    } else if (meetOp == UNION) {

      for(int i=0; i<bitvec_size; i++) {

        // T[i] = 0;
        B[i] = 1;

      }
    }

    // initial value of in and out 
    for (BasicBlock &B : *F) {

      if (direction) {
        in[&B] = T;
      } else {
        out[&B] = T;
      }
        // in[&B] = T;
        // out[&B] = T;

    }

    // Boundary condition

    setBoundary(direction, boundary_val, bitvec_size);



  }

  DFF::~DFF() {

  }  

  BBVal DFF::getIN() {

    return in;

  }

  BBVal DFF::getOUT() {

    return out;

  }

  void DFF::setGen(BBVal gen) {

    this->gen = gen;

  }

  void DFF::setKill(BBVal kill) {

    this->kill = kill;

  }

  void DFF::setLhs(BBVal glob_lhs) {

    this->glob_lhs = glob_lhs;

  }

  void DFF::setRhs(BBVal glob_rhs) {

    this->glob_rhs = glob_rhs;

  }
  void DFF::setUse(BBVal glob_use) {

    this->glob_use = glob_use;

  }

  void DFF::set_bvec_mapping(VMap mapping) {

    this->bvec_mapping = mapping;
  }

  BBList DFF::getPossibleExitBlocks() {

    BBList ret;

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

  BitVector DFF::applyMeet(BitVector b1, BitVector b2) {

    BitVector output;

    if (meetOp == INTERSECTION) {

      output = set_intersection(b1, b2);

    } else if (meetOp == UNION) {

      output = set_union(b1, b2);
    }
    return output;
  }

  void DFF::runAnalysis() {

    outs () << "********** Function: " + F->getName() + " ***********" << "\n";

    unsigned numIter = 0;

    if (direction) {

      // backward analysis
      // need exit block here but LLVM does not have an explicit exit block
      BBList exitBlockPreds = getPossibleExitBlocks();
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

            meetRes = in[succ];
            break;

          }
          for (BasicBlock *succ : successors(curr)) {

            meetRes = applyMeet(meetRes, in[succ]);

          }

          // Differentiate between separable and non-separable
          if(sep == SEPARABLE) {
            if (meetRes.size() > 0)
              out[curr] = meetRes;
            else
              out[curr] = in_exit;
          }

          // Update the gen kill sets
          else if(sep == NON_SEPARABLE) {
            
            // Store previous iteration
            BitVector lastIterOut = out[curr];

            // Calculate this iteration's out
            if (meetRes.size() > 0)
              out[curr] = meetRes;
            else
              out[curr] = in_exit;

            // Check if lastIterOut and this iter out match. 
            // No? Call analysis specific updater

            if(out[curr] != lastIterOut) {
              kill[curr] = updateDepKill(curr, kill[curr], out[curr], glob_lhs, glob_rhs, glob_use, bvec_mapping);
              gen[curr] = updateDepGen(curr, gen[curr], out[curr], glob_lhs, glob_rhs, glob_use, bvec_mapping);
            }

            
          }

          // push all succcessors in the queue
          for (BasicBlock *pred : predecessors(curr)) {

            if (visited.find(pred) == visited.end()) {

              bfs.push(pred);

            }
          }

          BitVector old_in = in[curr];
          BitVector old_out = out[curr];

          // call transfer function
          BitVector new_in = transferFunc(out[curr], gen[curr], kill[curr]);
          in[curr] = new_in;

          // compare the new in and out to the old ones
          if (new_in != old_in) 
            changed = true;

        }

        numIter++;
      } while (changed);


    } else {

      //reverse post order for fastest convergence. Can implement this too
      // const BasicBlock *entry = &F.getEntryBlock();

      // for (po_iterator<BasicBlock*> I = po_begin(&F.getEntryBlock()), IE = po_end(&F.getEntryBlock()); I != IE; ++I) {
      
      // }

      // forward analysis
      // breadth-first search solution
      BasicBlock *entry_block = &F->getEntryBlock();

      bool changed = false;

      do {

        changed = false;
        std::queue<BasicBlock*> bfs;
        DenseSet<BasicBlock*> visited;
        bfs.push(entry_block);

        while (!bfs.empty()) {

          BasicBlock *curr = bfs.front();
          bfs.pop();

          visited.insert(curr);

          // apply meet operator here
          BitVector meetRes;
          for (BasicBlock *pred : predecessors(curr)) {

            meetRes = out[pred];
            break;

          }
          for (BasicBlock *pred : predecessors(curr)) {

            meetRes = applyMeet(meetRes, out[pred]);

          }

          if (meetRes.size() > 0)
            in[curr] = meetRes;
          else
            in[curr] = out_entry;

          // push all succcessors in the queue
          for (BasicBlock *succ : successors(curr)) {

            if (visited.find(succ) == visited.end()) {

              bfs.push(succ);

            }
          }

          BitVector old_in = in[curr];
          BitVector old_out = out[curr];

          // call transfer function
          BitVector new_out = transferFunc(in[curr], gen[curr], kill[curr]);
          out[curr] = new_out;

          // compare the new in and out to the old ones
          if (new_out != old_out) 
            changed = true;

        }

        numIter++;
      } while (changed);
    }

    outs() << "Convergence: " << numIter << " iterations" <<"\n";
  }









  //overloaded print functions. Must be defined by analysis specific classes
  void DFF::print(BitVector b, Value *rev_mapping[]) {
    for (int i=0; i<b.size(); i++) {

      if (b[i])
        outs() << rev_mapping[i]->getName() << ",  ";
    }

    outs () << "\n";
  }








  // definitions of set operations
  BitVector set_union(BitVector b1, BitVector b2) {

    unsigned size = b1.size();
    BitVector u = BitVector(size);

    for (int i=0; i<size; i++) {

        u[i] = b1[i] || b2[i];
    }

    return u;
  }

  BitVector set_intersection(BitVector b1, BitVector b2) {

    unsigned size = b1.size();
    BitVector u = BitVector(size);

    for (int i=0; i<size; i++) {

        u[i] = b1[i] && b2[i];
    }

    return u;

  }

  BitVector set_diff(BitVector b1, BitVector b2) {

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
}
