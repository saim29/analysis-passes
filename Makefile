INC=-I/usr/local/include/
all: dominators.so dead-code-elimination.so loop-invariant-code-motion.so

CXXFLAGS = -rdynamic $(shell llvm-config --cxxflags) $(INC) -g -O0 -fPIC

dataflow.o: dataflow.cpp dataflow.h

dataflow_dce.o: dataflow_dce.cpp dataflow_dce.h

available-support.o: available-support.cpp available-support.h	

%.so: %.o dataflow.o available-support.o dataflow_dce.o
	$(CXX) -dylib -shared $^ -o $@

clean:
	rm -f *.o *~ *.so

.PHONY: clean all
