# data-flow-analyzer
Analyze data flow graph for C/C++/Fortran (maybe) programs.

# Build
```
LLVM_DIR=path/to/llvm/lib/cmake/llvm cmake ..
```


# Test
```
clang -c -g -emit-llvm -O2  src/plain/01.solver_seq_plain.c -o src/plain/01.solver_seq_plain.o
...
llvm-link src/plain/01.solver_seq_plain.o src/common/common.o src/plain/utils.o src/plain/main.o -o 01.nbody_seq_plain.N2.bc
opt -load /home/jinyuyang/PACMAN_PROJECT/huawei21/data-flow-analyzer/build/src/DFGPass.so -DFGPass 01.nbody_seq_plain.N2.bc -enable-new-pm=0 -o 01.nbody_seq_plain.N2.opt.bc
```
