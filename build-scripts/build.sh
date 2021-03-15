#!/bin/bash

# Build script intended for use in Travis CI

set -exo pipefail

num_jobs=3

function run_tests
{
    # The grep suppresses lines that begin with "0.0## s:", which are timing lines for tests with a very short duration.
    $WINE "$@" -d yes --use-colour yes --rng-seed time $EXTRA_TEST_OPTS | grep -Ev "^0\.0[0-9]{2} s:"
}

# We might need binaries installed via pip, so ensure that our personal bin dir is on the PATH
export PATH=$HOME/.local/bin:$PATH

# In OSX, gettext isn't in PATH
if [ -n "BREWGETTEXT" ]
then
    export PATH="/usr/local/opt/gettext/bin:$PATH"
fi

if [ -n "$DEPLOY" ]
then
    : # No-op, for now
elif [ -n "$TEST_STAGE" ]
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
        set -x

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
    # content rather than the size+mtime (which is ccache's default behavior).
    export CCACHE_COMPILERCHECK=content

    if [ "$ANDROID32" == 1 ]
    then
        ANDROID_ABI="-Pabi_arm_32=true -Pabi_arm_64=false"
    else
        ANDROID_ABI="-Pabi_arm_32=false -Pabi_arm_64=true"
    fi

    cd android
    # Keystore properties for signing the apk
    if [[ -n $encrypted_1de50180da55_key && -n $encrypted_f0d9c067c540_key ]]
    then
        openssl aes-256-cbc -K $encrypted_1de50180da55_key -iv $encrypted_1de50180da55_iv -in keystore.properties.enc -out keystore.properties -d
        openssl aes-256-cbc -K $encrypted_f0d9c067c540_key -iv $encrypted_f0d9c067c540_iv -in app/bn-release.keystore.enc -out app/bn-release.keystore -d
    fi
    # Specify dumb terminal to suppress gradle's constant output of time spent building, which
    # fills the log with nonsense.
    TERM=dumb ./gradlew assembleExperimentalRelease -Pj=$num_jobs $ANDROID_ABI -Poverride_version=${TRAVIS_BUILD_NUMBER} \
        -Pdeps="${HOME}/build/${TRAVIS_REPO_SLUG}/android/app/deps.zip"
else
    if [ "$DEPLOY" == 1 -a "$NATIVE" == "osx" ]
    then
        MAKE_TARGETS="all dmgdist"
    elif [ "$DEPLOY" == 1 ]
    then
        MAKE_TARGETS="all bindist"
    else
        MAKE_TARGETS="all"
    fi

    make -j "$num_jobs" RELEASE=1 CCACHE=1 BACKTRACE=1 LANGUAGES="all" CROSS="$CROSS_COMPILATION" LINTJSON=0 ${MAKE_TARGETS}

    if [ "$TRAVIS_OS_NAME" == "osx" ]
    then
        run_tests ./tests/cata_test
    elif [ -n "$MODS" ]
    then
        run_tests ./tests/cata_test --user-dir=modded $MODS
    else
        run_tests ./tests/cata_test
    fi

    if [ -n "$TEST_STAGE" ]
    then
        # Run the tests one more time, without actually running any tests, just to verify that all
        # the mod data can be successfully loaded

        # Use a blacklist of mods that currently fail to load cleanly.  Hopefully this list will
        # shrink over time.
        blacklist=build-scripts/mod_test_blacklist
        mods="$(./build-scripts/get_all_mods.py $blacklist)"
        run_tests ./tests/cata_test --user-dir=all_modded --mods="$mods" '~*'
    fi
fi
ccache --show-stats

# vim:tw=0
