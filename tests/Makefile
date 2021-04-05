# Makefile to make tests

#add any new tests to this variable

outputs = test_licm1.bc test_licm2.bc test_licm3.bc

all: $(outputs)

%.bc : %.c
	clang -Xclang -disable-O0-optnone -O0 -emit-llvm -c $< 

%_simplified.bc : %.bc
	opt -mem2reg -loop-simplify -loop-rotate -f $< -o $@
	opt -dot-cfg $@
	dot -Tpng .main.dot -o main.png

%_licm.bc : %_simplified.bc
	opt -load ../loop-invariant-code-motion.so -LICM -f $< -o $@
	opt -dot-cfg $@
	dot -Tpng .main.dot -o main-LICM.png


pass:
	make -C ..

clean:
	rm -f *.bc *.dot *.png *.ll
