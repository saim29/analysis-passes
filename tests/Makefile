# Makefile to make tests

#add any new tests to this variable

licm_tests = test_licm1_licm.bc test_licm2_licm.bc test_licm3_licm.bc
dce_tests = test_dce1_dce.bc test_dce2_dce.bc

outputs = $(licm_tests) $(dce_tests)

all: $(outputs)

%.bc : %.c
	clang -Xclang -disable-O0-optnone -O0 -emit-llvm -c $< 

%_m2r.bc : %.bc
	opt -mem2reg -f $< -o $@

%_simplified.bc : %_m2r.bc
	opt -loop-simplify -loop-rotate -f $< -o $@
	# opt -dot-cfg $@
	# dot -Tpng .main.dot -o main.png

%_licm.bc : %_simplified.bc pass
	opt -load ../loop-invariant-code-motion.so -LICM -f $< -o $@
	# opt -dot-cfg $@
	# dot -Tpng .main.dot -o main-LICM.png

%_dce.bc : %_m2r.bc pass
	opt -load ../dead-code-elimination.so -DCE -f $< -o $@
	# opt -dot-cfg $@
	# dot -Tpng .main.dot -o main.png

pass:
	make -C ..

clean:
	make clean -C ..
	rm -f *.bc *.dot *.png *.ll
