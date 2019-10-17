#!/bin/bash

# Build script intended for use in Travis CI

set -ex

num_jobs=3

function run_tests
{
    $WINE "$@" -d yes --rng-seed time $EXTRA_TEST_OPTS
}

date +%s > build-start-time

if [ -n "$TEST_STAGE" ]
then
    build-scripts/lint-json.sh
    make -j "$num_jobs" style-json

    tools/dialogue_validator.py data/json/npcs/* data/json/npcs/*/* data/json/npcs/*/*/*
    # Also build chkjson (even though we're not using it), to catch any
    # compile errors there
    make -j "$num_jobs" chkjson
elif [ -n "$JUST_JSON" ]
then
    echo "Early exit on just-json change"
    exit 0
fi

ccache --zero-stats
# Increase cache size because debug builds generate large object files
ccache -M 2G
ccache --show-stats

if [ -n "$CMAKE" ]
then
    bin_path="./"
    if [ "$RELEASE" = "1" ]
    then
        build_type=MinSizeRel
        bin_path="build/tests/"
    else
        build_type=Debug
    fi

    cmake_extra_opts=

    if [ "$CATA_CLANG_TIDY" = "plugin" ]
    then
        cmake_extra_opts="$cmake_extra_opts -DCATA_CLANG_TIDY_PLUGIN=ON"
        # Need to specify the particular LLVM / Clang versions to use, lest it
        # use the llvm-7 that comes by default on the Travis Xenial image.
        cmake_extra_opts="$cmake_extra_opts -DLLVM_DIR=/usr/lib/llvm-8/lib/cmake/llvm"
        cmake_extra_opts="$cmake_extra_opts -DClang_DIR=/usr/lib/llvm-8/lib/cmake/clang"
    fi

    mkdir build
    cd build
    cmake \
        -DBACKTRACE=ON \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DCMAKE_BUILD_TYPE="$build_type" \
        -DTILES=${TILES:-0} \
        -DSOUND=${SOUND:-0} \
        $cmake_extra_opts \
        ..
    if [ -n "$CATA_CLANG_TIDY" ]
    then
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

        # Run clang-tidy analysis instead of regular build & test
        # We could use CMake to create compile_commands.json, but that's super
        # slow, so use compiledb <https://github.com/nickdiego/compiledb>
        # instead.
        compiledb -n make

        cd ..
        ln -s build/compile_commands.json

        # We want to first analyze all files that changed in this PR, then as
        # many others as possible, in a random order.
        all_cpp_files="$( \
            grep '"file": "' build/compile_commands.json | \
            sed "s+.*$PWD/++;s+\"$++")"
        changed_cpp_files="$( \
            ./build-scripts/files_changed | grep -F "$all_cpp_files" || true )"
        if [ -n "$changed_cpp_files" ]
        then
            remaining_cpp_files="$( \
                echo "$all_cpp_files" | grep -v -F "$changed_cpp_files" || true )"
        else
            remaining_cpp_files="$all_cpp_files"
        fi

        function analyze_files_in_random_order
        {
            if [ -n "$1" ]
            then
                echo "$1" | shuf | xargs -P "$num_jobs" -n 1 ./build-scripts/clang-tidy-wrapper.sh
            else
                echo "No files to analyze"
            fi
        }

        echo "Analyzing changed files"
        analyze_files_in_random_order "$changed_cpp_files"

        echo "Analyzing remaining files"
        analyze_files_in_random_order "$remaining_cpp_files"
    else
        # Regular build
        make -j$num_jobs
        cd ..
        # Run regular tests
        [ -f "${bin_path}cata_test" ] && run_tests "${bin_path}cata_test"
        [ -f "${bin_path}cata_test-tiles" ] && run_tests "${bin_path}cata_test-tiles"
    fi
elif [ "$NATIVE" == "android" ]
then
    export USE_CCACHE=1
    export NDK_CCACHE="$(which ccache)"

    # Tweak the ccache compiler analysis.  We're using the compiler from the
    # Android NDK which has an unpredictable mtime, so we need to hash the
    # content rather than the size+mtime (which is ccache's default behaviour).
    export CCACHE_COMPILERCHECK=content

    cd android
    # Specify dumb terminal to suppress gradle's constatnt output of time spent building, which
    # fills the log with nonsense.
    TERM=dumb ./gradlew assembleRelease -Pj=$num_jobs -Plocalize=false -Pabi32=false -Pabi64=true -Pdeps=/home/travis/build/CleverRaven/Cataclysm-DDA/android/app/deps.zip -Poverride_version=0.D-Travis -Pversion_header_path=/home/travis/build/CleverRaven/Cataclysm-DDA/android/app/jni/src/version.h
else
    make -j "$num_jobs" RELEASE=1 CCACHE=1 BACKTRACE=1 CROSS="$CROSS_COMPILATION" LINTJSON=0

    if [ "$TRAVIS_OS_NAME" == "osx" ]
    then
        run_tests ./tests/cata_test
    else
        run_tests ./tests/cata_test &
        if [ -n "$MODS" ]
        then
            run_tests ./tests/cata_test --user-dir=modded $MODS &
            wait -n
        fi
        wait -n
    fi
fi
ccache --show-stats

# vim:tw=0
