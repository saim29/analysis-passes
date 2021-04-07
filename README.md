# Assignment 3 - Programming part
LLVM passes for dominators, dead code elimination and LICM utilizing framework (with modifications) from Assignment 2

## Team Members - Saim Ahmad, Tanmaya Mishra

## Folder Layout:
<pre>
.
├── available-support.cpp
├── available-support.h
├── dataflow.cpp
├── dataflow_dce.cpp
├── dataflow_dce.h
├── dataflow.h
├── dead-code-elimination.cpp
├── dominators.cpp
├── loop-invariant-code-motion.cpp
├── Makefile
├── README.md
└── tests
    ├── Makefile
    ├── test1.c
    ├── test2.c
    ├── test_dce1.c
    ├── test_dce2.c
    ├── test_licm1.c
    ├── test_licm2.c
    └── test_licm3.c

</pre>

## To build and run:

1. Change directory to tests/- `cd tests/`
2. Use Makefile in the following manner

    `make $name_$pass`
    
    - `$name` - Name of the source file without `.c` extension
    - `$pass` - Name of the pass. Possible: `dce`, `licm`

    Example build

    `make test_dce1_dce` to build `test_dce1.c`

   
3. Example output

```
$ make test_dce1_dce.bc
make -C ..
make[1]: Entering directory '/home/user/analysis-passes'
g++ -rdynamic -I/usr/local/include -std=c++14   -fno-exceptions -D_GNU_SOURCE -D_DEBUG -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -I/usr/local/include/ -g -O0 -fPIC   -c -o dataflow.o dataflow.cpp
g++ -rdynamic -I/usr/local/include -std=c++14   -fno-exceptions -D_GNU_SOURCE -D_DEBUG -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -I/usr/local/include/ -g -O0 -fPIC   -c -o available-support.o available-support.cpp
g++ -rdynamic -I/usr/local/include -std=c++14   -fno-exceptions -D_GNU_SOURCE -D_DEBUG -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -I/usr/local/include/ -g -O0 -fPIC   -c -o dataflow_dce.o dataflow_dce.cpp
g++ -rdynamic -I/usr/local/include -std=c++14   -fno-exceptions -D_GNU_SOURCE -D_DEBUG -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -I/usr/local/include/ -g -O0 -fPIC   -c -o dominators.o dominators.cpp
g++ -dylib -shared dominators.o dataflow.o available-support.o dataflow_dce.o -o dominators.so
g++ -rdynamic -I/usr/local/include -std=c++14   -fno-exceptions -D_GNU_SOURCE -D_DEBUG -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -I/usr/local/include/ -g -O0 -fPIC   -c -o dead-code-elimination.o dead-code-elimination.cpp
g++ -dylib -shared dead-code-elimination.o dataflow.o available-support.o dataflow_dce.o -o dead-code-elimination.so
g++ -rdynamic -I/usr/local/include -std=c++14   -fno-exceptions -D_GNU_SOURCE -D_DEBUG -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -I/usr/local/include/ -g -O0 -fPIC   -c -o loop-invariant-code-motion.o loop-invariant-code-motion.cpp
g++ -dylib -shared loop-invariant-code-motion.o dataflow.o available-support.o dataflow_dce.o -o loop-invariant-code-motion.so
rm loop-invariant-code-motion.o dominators.o dead-code-elimination.o
make[1]: Leaving directory '/home/user/analysis-passes'
clang -Xclang -disable-O0-optnone -O0 -emit-llvm -c test_dce1.c
opt -mem2reg -f test_dce1.bc -o test_dce1_m2r.bc
opt -load ../dead-code-elimination.so -DCE -f test_dce1_m2r.bc -o test_dce1_dce.bc
********** Function: main ***********
Convergence: 2 iterations
============================
  %add = add nsw i32 100, 100
============================

IN: 
  %add = add nsw i32 100, 100
  %mul = mul nsw i32 25, 4
  %add1 = add nsw i32 %add, %mul
  %add2 = add nsw i32 5, 5
  %div = sdiv i32 %add, 25


gen
  %add = add nsw i32 100, 100


kill


OUT: 
  %mul = mul nsw i32 25, 4
  %add1 = add nsw i32 %add, %mul
  %add2 = add nsw i32 5, 5
  %div = sdiv i32 %add, 25


LHS: 
  %add = add nsw i32 100, 100


RHS: 


USE: 


====================================
============================
  %mul = mul nsw i32 25, 4
============================

IN: 
  %mul = mul nsw i32 25, 4
  %add1 = add nsw i32 %add, %mul
  %add2 = add nsw i32 5, 5
  %div = sdiv i32 %add, 25


gen
  %mul = mul nsw i32 25, 4


kill


OUT: 
  %mul = mul nsw i32 25, 4
  %add1 = add nsw i32 %add, %mul
  %add2 = add nsw i32 5, 5
  %div = sdiv i32 %add, 25


LHS: 
  %mul = mul nsw i32 25, 4


RHS: 


USE: 


====================================
============================
  %add1 = add nsw i32 %add, %mul
============================

IN: 
  %mul = mul nsw i32 25, 4
  %add1 = add nsw i32 %add, %mul
  %add2 = add nsw i32 5, 5
  %div = sdiv i32 %add, 25


gen
  %add1 = add nsw i32 %add, %mul


kill


OUT: 
  %mul = mul nsw i32 25, 4
  %add1 = add nsw i32 %add, %mul
  %add2 = add nsw i32 5, 5
  %div = sdiv i32 %add, 25


LHS: 
  %add1 = add nsw i32 %add, %mul


RHS: 
  %add = add nsw i32 100, 100
  %mul = mul nsw i32 25, 4


USE: 


====================================
============================
  %add2 = add nsw i32 5, 5
============================

IN: 
  %mul = mul nsw i32 25, 4
  %add1 = add nsw i32 %add, %mul
  %add2 = add nsw i32 5, 5
  %div = sdiv i32 %add, 25


gen
  %add2 = add nsw i32 5, 5


kill


OUT: 
  %mul = mul nsw i32 25, 4
  %add1 = add nsw i32 %add, %mul
  %add2 = add nsw i32 5, 5
  %div = sdiv i32 %add, 25


LHS: 
  %add2 = add nsw i32 5, 5


RHS: 


USE: 


====================================
============================
  %div = sdiv i32 %add, 25
============================

IN: 
  %mul = mul nsw i32 25, 4
  %add1 = add nsw i32 %add, %mul
  %add2 = add nsw i32 5, 5
  %div = sdiv i32 %add, 25


gen
  %div = sdiv i32 %add, 25


kill


OUT: 
  %add = add nsw i32 100, 100
  %mul = mul nsw i32 25, 4
  %add1 = add nsw i32 %add, %mul
  %add2 = add nsw i32 5, 5


LHS: 
  %div = sdiv i32 %add, 25


RHS: 
  %add = add nsw i32 100, 100


USE: 


====================================
============================
  ret i32 %div
============================

IN: 
  %add = add nsw i32 100, 100
  %mul = mul nsw i32 25, 4
  %add1 = add nsw i32 %add, %mul
  %add2 = add nsw i32 5, 5


gen


kill
  %div = sdiv i32 %add, 25


OUT: 
  %add = add nsw i32 100, 100
  %mul = mul nsw i32 25, 4
  %add1 = add nsw i32 %add, %mul
  %add2 = add nsw i32 5, 5
  %div = sdiv i32 %add, 25


LHS: 


RHS: 


USE: 
  %div = sdiv i32 %add, 25


====================================
opt -dot-cfg test_dce1_dce.bc
Writing '.main.dot'...
dot -Tpng .main.dot -o main.png
rm test_dce1.bc
```


# NOTE

We also generate dot and png files of the resultant bitcode files for visualization. They will be available in the `tests/` folder.