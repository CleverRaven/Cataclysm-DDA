#!/bin/bash

# Shell script intended for clang-tidy check

echo "Using bash version $BASH_VERSION"
set -exo pipefail

num_jobs=3

# We might need binaries installed via pip, so ensure that our personal bin dir is on the PATH
#export PATH=$HOME/.local/bin:$PATH
build_type=MinSizeRel

cmake_extra_opts=()
cmake_extra_opts+=("-DCATA_CLANG_TIDY_PLUGIN=ON")
# Need to specify the particular LLVM / Clang versions to use, lest it
# use the older LLVM that comes by default on Ubuntu.
cmake_extra_opts+=("-DLLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm")
cmake_extra_opts+=("-DClang_DIR=/usr/lib/llvm-18/lib/cmake/clang")


mkdir -p build
cd build
cmake \
    ${COMPILER:+-DCMAKE_CXX_COMPILER=$COMPILER} \
    -DCMAKE_BUILD_TYPE="$build_type" \
    -DLOCALIZE=OFF \
    "${cmake_extra_opts[@]}" \
    ..


echo "Compiling clang-tidy plugin"
make -j$num_jobs CataAnalyzerPlugin
#export PATH=$PWD/tools/clang-tidy-plugin/clang-tidy-plugin-support/bin:$PATH
# add FileCheck to the search path
export PATH=/usr/lib/llvm-18/bin:$PATH 
if ! which FileCheck
then
    echo "Missing FileCheck"
    exit 1
fi
CATA_CLANG_TIDY=clang-tidy
# lit might be installed via pip, so ensure that our personal bin dir is on the PATH
export PATH=$HOME/.local/bin:$PATH
lit -v tools/clang-tidy-plugin/test
cd ..

# show that it works

echo "version:"
LD_PRELOAD=build/tools/clang-tidy-plugin/libCataAnalyzerPlugin.so clang-tidy --version
echo "all enabled checks:"
LD_PRELOAD=build/tools/clang-tidy-plugin/libCataAnalyzerPlugin.so clang-tidy --list-checks
echo "cata-specific checks:"
LD_PRELOAD=build/tools/clang-tidy-plugin/libCataAnalyzerPlugin.so clang-tidy --list-checks --checks="cata-*" | grep "cata"

