#!/bin/bash

# Shell script intended for clang-tidy check

echo "Using bash version $BASH_VERSION"
set -exo pipefail

num_jobs=3

# We might need binaries installed via pip, so ensure that our personal bin dir is on the PATH
export PATH=$HOME/.local/bin:$PATH

if [ "$RELEASE" = "1" ]
then
    build_type=MinSizeRel
else
    build_type=Debug
fi

cmake_extra_opts=()

if [ "$CATA_CLANG_TIDY" = "plugin" ]
then
    cmake_extra_opts+=("-DCATA_CLANG_TIDY_PLUGIN=ON")
    # Need to specify the particular LLVM / Clang versions to use, lest it
    # use the llvm-7 that comes by default on the Travis Xenial image.
    cmake_extra_opts+=("-DLLVM_DIR=/usr/lib/llvm-12/lib/cmake/llvm")
    cmake_extra_opts+=("-DClang_DIR=/usr/lib/llvm-12/lib/cmake/clang")
fi

if [ "$COMPILER" = "clang++-12" -a -n "$GITHUB_WORKFLOW" -a -n "$CATA_CLANG_TIDY" ]
then
    # This is a hacky workaround for the fact that the custom clang-tidy we are
    # using is built for Travis CI, so it's not using the correct include directories
    # for GitHub workflows.
    cmake_extra_opts+=("-DCMAKE_CXX_FLAGS=-isystem /usr/include/clang/12.0.0/include")
fi

mkdir -p build
cd build
cmake \
    -DBACKTRACE=ON \
    ${COMPILER:+-DCMAKE_CXX_COMPILER=$COMPILER} \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DCMAKE_BUILD_TYPE="$build_type" \
    -DTILES=${TILES:-0} \
    -DSOUND=${SOUND:-0} \
    "${cmake_extra_opts[@]}" \
    ..

if [ "$CATA_CLANG_TIDY" = "plugin" ]
then
    make -j$num_jobs CataAnalyzerPlugin
    export PATH=$PWD/tools/clang-tidy-plugin/clang-tidy-plugin-support/bin:$PATH
    if ! which FileCheck
    then
        ls -l tools/clang-tidy-plugin/clang-tidy-plugin-support/bin
        ls -l /usr/bin
        echo "Missing FileCheck"
        exit 1
    fi
    CATA_CLANG_TIDY=clang-tidy
    lit -v tools/clang-tidy-plugin/test
fi

"$CATA_CLANG_TIDY" --version

# Show compiler C++ header search path
${COMPILER:-clang++} -v -x c++ /dev/null -c
# And the same for clang-tidy
"$CATA_CLANG_TIDY" ../src/version.cpp -- -v

# Run clang-tidy analysis instead of regular build & test
# We could use CMake to create compile_commands.json, but that's super
# slow, so use compiledb <https://github.com/nickdiego/compiledb>
# instead.
compiledb -n make

cd ..
rm -f compile_commands.json && ln -s build/compile_commands.json

# We want to first analyze all files that changed in this PR, then as
# many others as possible, in a random order.
set +x

# Check for changes to any files that would require us to run clang-tidy across everything
changed_global_files="$( ( cat ./files_changed || echo 'unknown' ) | \
    egrep -i "clang-tidy|build-scripts|cmake|unknown" || true )"
if [ -n "$changed_global_files" ]
then
    first_changed_file="$(echo "$changed_global_files" | head -n 1)"
    echo "Analyzing all files because $first_changed_file was changed"
    TIDY="all"
fi

all_cpp_files="$( \
    grep '"file": "' build/compile_commands.json | \
    sed "s+.*$PWD/++;s+\"$++")"
if [ "$TIDY" == "all" ]
then
    echo "Analyzing all files"
    tidyable_cpp_files=$all_cpp_files
else
    make \
        -j $num_jobs \
        ${COMPILER:+COMPILER=$COMPILER} \
        TILES=${TILES:-0} \
        SOUND=${SOUND:-0} \
        includes

    tidyable_cpp_files="$( \
        ( test -f ./files_changed && build-scripts/get_affected_files.py ./files_changed ) || \
        echo unknown )"

    if [ "tidyable_cpp_files" == "unknown" ]
    then
        echo "Unable to determine affected files, tidying all files"
        tidyable_cpp_files=$all_cpp_files
    fi
fi

function analyze_files_in_random_order
{
    if [ -n "$1" ]
    then
        echo "$1" | shuf | \
            xargs -P "$num_jobs" -n 1 ./build-scripts/clang-tidy-wrapper.sh -quiet
    else
        echo "No files to analyze"
    fi
}

echo "Analyzing affected files"
analyze_files_in_random_order "$tidyable_cpp_files"
set -x
