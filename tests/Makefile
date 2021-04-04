# Makefile to make tests

#add any new tests to this variable
outputs = test_licm-simple.bc

all: $(outputs)

test_licm.bc : test_licm.c
	clang -Xclang -disable-O0-optnone -O0 -emit-llvm -c $< 

test_licm-simple.bc : test_licm.bc
	opt -mem2reg -loop-simplify $< -o $@
	opt -dot-cfg $@
	dot -Tpng .main.dot -o main.png
	dot -Tpng .test.dot -o test.png

# %.bc : %.c
# 	clang -c -O -Xclang -disable-llvm-passes -emit-llvm $<

clean:
	sudo rm -r *.bc *.dot *.png *.ll