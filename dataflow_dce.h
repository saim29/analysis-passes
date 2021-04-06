
////////////////////////////////////////////////////////////////////////////////

#ifndef __CLASSICAL_DATAFLOW_H__
#define __CLASSICAL_DATAFLOW_H__

#include <stdio.h>
#include <iostream>
#include <queue>
#include <stack>
#include <vector>

#include "llvm/IR/Instructions.h"
#include "llvm/ADT/BitVector.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/ValueMap.h"
#include "llvm/IR/CFG.h"
#include "llvm/Pass.h"

// included for convenience
#include "llvm/ADT/PostOrderIterator.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/IR/DerivedTypes.h"

#include "available-support.h"

namespace llvm {

    // Add definitions (and code, depending on your strategy) for your dataflow
    // abstraction here.

    // set operations for bitvectors
    BitVector set_union(BitVector b1, BitVector b2);
    BitVector set_intersection(BitVector b1, BitVector b2);
    BitVector set_diff(BitVector b1, BitVector b2);

    // can add support for more meet operators here
    enum meetOperator {

        INTERSECTION,
        UNION

    };

    enum separability {
        SEPARABLE,
        NON_SEPARABLE
    };

    /*typedef DenseMap <BasicBlock*, BitVector> BBVal;
    typedef std::map <Value*, unsigned> VMap;
    typedef std::map <Expression*, unsigned> EMap;*/
    typedef std::vector<BasicBlock*> BBList;
    /*typedef BitVector (*transferFuncTy) (BitVector, BitVector, BitVector);
    typedef BitVector(*genKillUpdaterTy) (BasicBlock*, BitVector, BitVector, BBVal, BBVal, BBVal, VMap);*/


    typedef DenseMap <Instruction*, BitVector> IVal;
    typedef std::map <Value*, unsigned> VMap;
    typedef std::map <Expression*, unsigned> EMap;
    typedef std::vector<Instruction*> IList;
    typedef BitVector (*transferFuncTy) (BitVector, BitVector, BitVector);
    typedef BitVector(*genKillUpdaterTy) (Instruction*, BitVector, BitVector, IVal, IVal, IVal, VMap);
    

    class DFF {

        private:

        Function *F; // pointer to the function under inspection

        VMap bvec_mapping;

        bool direction; // 0 forward; 1 backward
        meetOperator meetOp; // meet operator for preds or succ
        separability sep; // separability condition for analysis like faint analysis

        IVal in; // in[B]
        IVal out; // out[B]

        IVal glob_lhs; 
        IVal glob_rhs; 
        IVal glob_use;
        
        // gen and kill sets; Should be calculated by the specific analysis and passed to DFF
        IVal gen;
        IVal kill;

        BitVector T; // Top value of the semi lattice
        BitVector B; // Bottom value of the semi lattice

        // bit vectors for unique (pseudo) ENTRY and EXIT blocks
        BitVector in_exit;
        BitVector out_entry;

        BitVector (*transferFunc)(BitVector, BitVector, BitVector); // function pointer to the transfer function of the analysis class

        BitVector applyMeet(BitVector b1, BitVector b2); //function to apply meet 

        // function to generate possible return blocks
        BBList getPossibleExitBlocks();

        public:
        // constructors for DFF
        DFF();
        DFF(Function *F, bool direction, meetOperator meetOp, unsigned bitvec_size, transferFuncTy transferFunc,
           bool boundary_val, separability sep, genKillUpdaterTy depGen, genKillUpdaterTy depKill);

        // methods to set specific sets
        void setGen(IVal gen);
        void setKill(IVal kill);

        void set_bvec_mapping(VMap mapping);

        // Sets for faint variable analysis

        void setLhs(IVal glob_lhs);
        void setRhs(IVal glob_rhs);
        void setUse(IVal glob_use);


        void setBoundary(bool direction, bool boundary_val, unsigned bitvec_size);

        // methods to return the result
        IVal getIN();
        IVal getOUT();

        // function to print all results on convergence
        template<class A> 
        void printRes(std::map<A, unsigned> mapping, StringRef label1, StringRef label2);

        // overloaded print functions
        void print(BitVector b, Value *rev_mapping[]); 

        
        BitVector (*updateDepGen)(BasicBlock *B, BitVector gen, BitVector out, IVal lhs, IVal rhs, IVal use, VMap bmap);
        BitVector (*updateDepKill)(BasicBlock *B, BitVector kill, BitVector out, IVal lhs, IVal rhs, IVal use, VMap bmap);
        // destructor for DFF
        ~DFF();

        void runAnalysis(); // traversal of basicblocks based on the direction boolean

    };


// print functions moved here. C++ requirement
  template<class A> 
  void DFF::printRes(std::map<A, unsigned> mapping, StringRef label1, StringRef label2) {

    A rev_mapping[mapping.size()];

    for (auto ele : mapping) {

      unsigned ind = ele.second;
      A val = ele.first;

      rev_mapping[ind] = val;

    }

    for (BasicBlock &B: *F) {

      StringRef bName = B.getName();

      outs () << "==============" + bName + "==============" << "\n";

      outs () << "\nIN: \n";
      print(in[&B], rev_mapping);

      outs () << "\n" << label1 << "\n";
      print(gen[&B], rev_mapping);

      outs () << "\n" << label2 << "\n";
      print(kill[&B], rev_mapping);

      outs () << "\nOUT: \n";
      print(out[&B], rev_mapping);

      outs () << "\n====================================" << "\n";

    }

  }


}

#endif
