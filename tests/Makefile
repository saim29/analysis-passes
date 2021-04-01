# Makefile to make tests

# add any new tests to this variable
outputs = test_dom.bc

all: $(outputs)

%.bc : %.c
	clang -c -O -Xclang -disable-llvm-passes -emit-llvm $<

clean:
	rm *.bc 