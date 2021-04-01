# analysis-passes

to make passes do:

make

To make tests do:

make -C tests

To run any pass:

opt -load {path to .so file} --{name of pass} {path to test .bc file}

example:
opt -load ./dominators.so --dominators ./tests/test_dom.bc
