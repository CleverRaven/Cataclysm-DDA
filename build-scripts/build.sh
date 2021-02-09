#!/bin/bash

# Build script intended for use in Travis CI and Github workflow

set -exo pipefail

num_jobs=3

# We might need binaries installed via pip, so ensure that our personal bin dir is on the PATH
export PATH=$HOME/.local/bin:$PATH

if [ -n "$TEST_STAGE" ]
then
    build-scripts/lint-json.sh
    make -j "$num_jobs" style-json

    tools/dialogue_validator.py data/json/npcs/* data/json/npcs/*/* data/json/npcs/*/*/*

    tools/json_tools/generic_guns_validator.py

    # Also build chkjson (even though we're not using it), to catch any
    # compile errors there
    make -j "$num_jobs" chkjson
# Skip the rest of the run if this change is pure json and this job doesn't test any extra mods
elif [ -n "$JUST_JSON" -a -z "$MODS" ]
then
    echo "Early exit on just-json change"
    exit 0
fi

ccache --zero-stats
# Increase cache size because debug builds generate large object files
ccache -M 5G
ccache --show-stats

if [ "$CMAKE" = "1" ]
then
    bin_path="./"
    if [ "$RELEASE" = "1" ]
    then
        build_type=MinSizeRel
        bin_path="build/tests/"
    else
        build_type=Debug
    fi

    cmake_extra_opts=()

    if [ "$CATA_CLANG_TIDY" = "plugin" ]
    then
        cmake_extra_opts+=("-DCATA_CLANG_TIDY_PLUGIN=ON")
        # Need to specify the particular LLVM / Clang versions to use, lest it
        # use the llvm-7 that comes by default on the Travis Xenial image.
        cmake_extra_opts+=("-DLLVM_DIR=/usr/lib/llvm-8/lib/cmake/llvm")
        cmake_extra_opts+=("-DClang_DIR=/usr/lib/llvm-8/lib/cmake/clang")
    fi

    if [ "$COMPILER" = "clang++-8" -a -n "$GITHUB_WORKFLOW" -a -n "$CATA_CLANG_TIDY" ]
    then
        # This is a hacky workaround for the fact that the custom clang-tidy we are
        # using is built for Travis CI, so it's not using the correct include directories
        # for GitHub workflows.
        cmake_extra_opts+=("-DCMAKE_CXX_FLAGS=-isystem /usr/include/clang/8.0.0/include")
    fi

    mkdir build
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
        ln -s build/compile_commands.json

        # We want to first analyze all files that changed in this PR, then as
        # many others as possible, in a random order.
        set +x
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
                echo "$1" | shuf | \
                    xargs -P "$num_jobs" -n 1 ./build-scripts/clang-tidy-wrapper.sh -quiet
            else
                echo "No files to analyze"
            fi
        }

        echo "Analyzing changed files"
        analyze_files_in_random_order "$changed_cpp_files"

        echo "Analyzing remaining files"
        analyze_files_in_random_order "$remaining_cpp_files"
        set -x
    else
        # Regular build
        make -j$num_jobs
        cd ..
        # Run regular tests
        [ -f "${bin_path}cata_test" ] && parallel --verbose --tagstring "({})=>" --linebuffer $WINE ${bin_path}/cata_test --min-duration 0.2 --use-colour yes --rng-seed time $EXTRA_TEST_OPTS ::: "crafting_skill_gain" "[slow] ~crafting_skill_gain" "~[slow] ~[.]"
        [ -f "${bin_path}cata_test-tiles" ] && parallel --verbose --tagstring "({})=>" --linebuffer $WINE ${bin_path}/cata_test-tiles --min-duration 0.2 --use-colour yes --rng-seed time $EXTRA_TEST_OPTS ::: "crafting_skill_gain" "[slow] ~crafting_skill_gain" "~[slow] ~[.]"
    fi
elif [ "$NATIVE" == "android" ]
then
    export USE_CCACHE=1
    export NDK_CCACHE="$(which ccache)"

    # Tweak the ccache compiler analysis.  We're using the compiler from the
    # Android NDK which has an unpredictable mtime, so we need to hash the
    # content rather than the size+mtime (which is ccache's default behavior).
    export CCACHE_COMPILERCHECK=content

    cd android
    # Specify dumb terminal to suppress gradle's constant output of time spent building, which
    # fills the log with nonsense.
    TERM=dumb ./gradlew assembleExperimentalRelease -Pj=$num_jobs -Plocalize=false -Pabi_arm_32=false -Pabi_arm_64=true -Pdeps=/home/travis/build/CleverRaven/Cataclysm-DDA/android/app/deps.zip
else
    if [ "$OS" == "macos-10.15" ]
    then
        # if OSX_MIN we specify here is lower than 10.15 then linker is going
        # to throw warnings because SDL and gettext libraries installed from 
        # Homebrew are built with minimum target osx version 10.15
        export OSX_MIN=10.15
    else
        export BACKTRACE=1
    fi
    make -j "$num_jobs" RELEASE=1 CCACHE=1 CROSS="$CROSS_COMPILATION" LINTJSON=0

    export ASAN_OPTIONS=detect_odr_violation=1
    export UBSAN_OPTIONS=print_stacktrace=1
    parallel --verbose --tagstring "({})=>" --linebuffer $WINE ./tests/cata_test --min-duration 0.2 --use-colour yes --rng-seed time $EXTRA_TEST_OPTS ::: "crafting_skill_gain" "[slow] ~crafting_skill_gain" "~[slow] ~[.]"
    if [ -n "$MODS" ]
    then
        parallel --verbose --tagstring "(Mods-{})=>" --linebuffer $WINE ./tests/cata_test --user-dir=modded $MODS --min-duration 0.2 --use-colour yes --rng-seed time $EXTRA_TEST_OPTS ::: "crafting_skill_gain" "[slow] ~crafting_skill_gain" "~[slow] ~[.]"
    fi

    if [ -n "$TEST_STAGE" ]
    then
        # Run the tests one more time, without actually running any tests, just to verify that all
        # the mod data can be successfully loaded

        mods="$(./build-scripts/get_all_mods.py)"
        $WINE ./tests/cata_test --user-dir=all_modded --mods="$mods" --min-duration 0.2 --use-colour yes --rng-seed time $EXTRA_TEST_OPTS "~*"
    fi
fi
ccache --show-stats
# Shrink the ccache back down to 2GB in preperation for pushing to shared storage.
ccache -M 2G
ccache -c

# vim:tw=0
