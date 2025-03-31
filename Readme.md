

# DESIL

Welcome to the home page of DESIL's repository, which contains three folders:

``src`` contains the source code of DESIL.

``exp`` contains the experiment script of DESIL.

``data`` contains the experiment data of DESIL.



# Build DESIL (src folder)

**Note that we integrated DESIL, MLIRSmith, MLIRod, and MLIR compiler infrastructure together into**
```src```
**, you can build all of them together!**

Use the following commands to build DESIL, MLIRSmith, MLIRod, and MLIR compiler infrastructure together:

```
cd src
mkdir build
cd build
cmake -G Ninja ../llvm \
  -DLLVM_ENABLE_PROJECTS=mlir \
  -DLLVM_BUILD_EXAMPLES=ON \
  -DLLVM_TARGETS_TO_BUILD="X86;NVPTX;AMDGPU" \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_ASSERTIONS=ON
cmake --build .
```

<!-- ---

You can use additional settings:

```
-DCMAKE_C_COMPILER=afl-cc \
-DCMAKE_CXX_COMPILER=afl-c++ \
```
to support AFL instrumentation for edge coverage collection.

---

or use settings:
```
-DCMAKE_C_FLAGS="-g -O0 -fprofile-arcs -ftest-coverage" \
-DCMAKE_CXX_FLAGS="-g -O0 -fprofile-arcs -ftest-coverage" \
-DCMAKE_EXE_LINKER_FLAGS="-g -fprofile-arcs -ftest-coverage -lgcov" \
```
to enable gcov for line coverage collection. -->

```src``` is developed based on LLVM repository (git version ```1d44ecb9daffbc3b1ed78b7c95662a6fea3f90b9```, not the version under evaluate ```c6d6da4659599507b44c167f335639082f28fae6```), and more detailed information about building MLIR compiler infrastructure can be found in https://mlir.llvm.org/getting_started/


# experiment script (exp folder)

```exp``` folder contains all experiment scripts, including:

+ MLIRSmith: which contains the experiment script of $MLIRSmith_{enhanced}$.

+ MLIRSmith_FIXUB: which contains the experiment script of $DESIL_{Smith}$.

+ MLIRSmith_FIXUB_no_pass_lowering: which contains the experiment script of $DESIL_{Smith}^{w/o\ low}$.

+ MLIRSmith_FIXUB_no_pass_recommend: which contains the experiment script of $DESIL_{Smith}^{w/o\ opt}$.

+ MLIRod: which contains the experiment script of $MLIRod_{enhanced}$.

+ MLIRod_FIXUB: which contains the experiment script of $DESIL_{od}$.

+ MLIRod_FIXUB_no_pass_lowering: which contains the experiment script of $DESIL_{od}^{w/o\ low}$.

+ MLIRod_FIXUB_no_pass_recommend: which contains the experiment script of $DESIL_{od}^{w/o\ opt}$.

All scripts in folder with prefix "MLIRSmith" can run with following comamnd:

```
timeout 43200 python3 MLIRSmith_FIXUB.py \
--diff_num 1 \
--opt_num 1 \
--init_size 3000 \
--lowering_iter_limit 50 \
--mlir_opt ${path_of_mlir_opt} \
--mlirsmith ${src_build_path}/bin/MLIRSmith \
--fixub ${src_build_path}/bin/FixUBnew \
--checksum ${src_build_path}/bin/CheckSum \
--clean True > log.log &
```

All script in folder with prefix "MLIRod" can run with following command:

```
timeout 43200 python3 MLIRod_FIXUB.py \
--seed existing \
--diff_num 1 \
--opt_num 1 \
--init_size 3000 \
--lowering_iter_limit 50 \
--existing_seed_dir existing_seeds \
--mlir_opt ${path_of_mlir_opt} \
--mlirsmith ${src_build_path}/bin/toyc-ch5_dynamicLowering \
--fixub ${src_build_path}/bin/FixUBnew \
--checksum ${src_build_path}/bin/CheckSum \
--replace_data_edge ${src_build_path}/bin/ReplaceDataEdge \
--replace_control_edge ${src_build_path}/bin/ReplaceControlEdge \
--delete_node ${src_build_path}/bin/DeleteNode \
--create_node ${src_build_path}/bin/CreateNode \
--collector ${src_build_path}/bin/CollectCoverage \
--clean True >log.log 2>error.log &
```

One can change the parameter ```diff_num``` and ```opt_num``` for parameter experiment.

# data folder 

```data``` folder contains all the experiment data (mainly bug-triggering programs) in paper, including:

+ UB_elimination: which contains the experiment data of DESIL.

+ no_elimination: which contains the experiment data of enhanced MLIRSmith and MLIRod.

+ no_lowering: which contains the experiment data of $DESIL_{Smith}^{w/o\ low}$ and $DESIL_{od}^{w/o\ low}$.

+ no_opt: which contains the experiment data of $DESIL_{Smith}^{w/o\ opt}$ and $DESIL_{od}^{w/o\ opt}$.

+ param: which contains the experiment data of all parameter experiment in paper.