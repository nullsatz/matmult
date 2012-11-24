OBJS := matmult.o

#compiler/preprocessor options
INCS := -I. -I/Developer/NVIDIA/CUDA-5.0/include -I/Library/Frameworks/R.framework/Versions/2.15/Resources/include -I/Library/Frameworks/R.framework/Resources/include/x86_64 -I/Library/Frameworks/R.framework/Versions/2.15/Resources/library/Rcpp/include
PARAMS :=  -m64 -Xcompiler -fPIC

#linker options
LD_PARAMS := -m64 -Xlinker -rpath,/Developer/NVIDIA/CUDA-5.0/lib,'-F/Library/Frameworks/R.framework/.. -framework R'
LIBS :=   -L/Library/Frameworks/R.framework/Versions/2.15/Resources/library/Rcpp/lib/x86_64 -lRcpp -L/Developer/NVIDIA/CUDA-5.0/lib -lcublas -lcudart

TARGETS := matmult.so

NVCC := /Developer/NVIDIA/CUDA-5.0/bin/nvcc -gencode arch=compute_10,code=sm_10 -gencode arch=compute_11,code=sm_11 -gencode arch=compute_12,code=sm_12 -gencode arch=compute_13,code=sm_13 -gencode arch=compute_20,code=sm_20

all: $(TARGETS) 

$(TARGETS): $(OBJS)
	$(NVCC) -shared $(LD_PARAMS) $(LIBS) $(OBJS) -o $@

$(OBJS): %.o: %.cpp
	$(NVCC) -c $(INCS) $(PARAMS) $^ -o $@

clean:
	rm -rf *o

.PHONY: all clean
