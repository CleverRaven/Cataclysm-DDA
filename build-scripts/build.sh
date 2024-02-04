#!/bin/bash

# Build script intended for use in Travis CI and Github workflow

echo "Using bash version $BASH_VERSION"
set -exo pipefail

num_jobs=3
[ -z $NUM_TEST_JOBS ] && num_test_jobs=3 || num_test_jobs=$NUM_TEST_JOBS

# We might need binaries installed via pip, so ensure that our personal bin dir is on the PATH
export PATH=$HOME/.local/bin:$PATH

$COMPILER --version
if [ -n "$CROSS_COMPILATION" ]
then
    "$CROSS_COMPILATION$COMPILER" --version
fi

if [ -n "$TEST_STAGE" ]
then
    build-scripts/validate_json.py
    make style-all-json-parallel RELEASE=1

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
ccache --show-stats --verbose

function run_test
{
    set -eo pipefail
    test_exit_code=0 sed_exit_code=0 exit_code=0
    test_bin=$1
    prefix=$2
    shift 2

    $WINE "$test_bin" --min-duration 0.2 --use-colour yes --rng-seed time $EXTRA_TEST_OPTS "$@" 2>&1 | sed -E 's/^(::(warning|error|debug)[^:]*::)?/\1'"$prefix"'/' || test_exit_code="${PIPESTATUS[0]}" sed_exit_code="${PIPESTATUS[1]}"
    if [ "$test_exit_code" -ne "0" ]
    then
        echo "$3test exited with code $test_exit_code"
        exit_code=1
    fi
    if [ "$sed_exit_code" -ne "0" ]
    then
        echo "$3sed exited with code $sed_exit_code"
        exit_code=1
    fi
    return $exit_code
}
export -f run_test

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
        -DLIBBACKTRACE=${LIBBACKTRACE:-0} \
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
        rm -f compile_commands.json && ln -s build/compile_commands.json

        set +x
        # Check for changes to any files that would require us to run clang-tidy across everything
        changed_global_files="$( ( ./build-scripts/files_changed || echo 'unknown') | \
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

            ./build-scripts/files_changed > ./files_changed
            tidyable_cpp_files="$( \
                ( build-scripts/get_affected_files.py ./files_changed ) || \
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
    else
        # Regular build
        make -j$num_jobs
        cd ..
        # Run regular tests
        [ -f "${bin_path}cata_test" ] && parallel --verbose --linebuffer "run_test $(printf %q "${bin_path}")'/cata_test' '('{}')=> ' --user-dir=test_user_dir_{#} {}" ::: "crafting_skill_gain" "[slow] ~crafting_skill_gain" "~[slow] ~[.]"
        [ -f "${bin_path}cata_test-tiles" ] && parallel --verbose --linebuffer "run_test $(printf %q "${bin_path}")'/cata_test-tiles' '('{}')=> ' --user-dir=test_user_dir_{#} {}" ::: "crafting_skill_gain" "[slow] ~crafting_skill_gain" "~[slow] ~[.]"
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
    make -j "$num_jobs" RELEASE=1 CCACHE=1 CROSS="$CROSS_COMPILATION" LINTJSON=0 LIBBACKTRACE="$LIBBACKTRACE"

    export ASAN_OPTIONS=detect_odr_violation=1
    export UBSAN_OPTIONS=print_stacktrace=1
    parallel -j "$num_test_jobs" --verbose --linebuffer "run_test './tests/cata_test' '('{}')=> ' --user-dir=test_user_dir_{#} {}" ::: "crafting_skill_gain" "[slow] ~crafting_skill_gain" "~[slow] ~[.]"
    if [ -n "$MODS" ]
    then
        parallel -j "$num_test_jobs" --verbose --linebuffer "run_test './tests/cata_test' 'Mods-('{}')=> ' $(printf %q "${MODS}") --user-dir=modded_{#} {}" ::: "crafting_skill_gain" "[slow] ~crafting_skill_gain" "~[slow] ~[.]"
    fi

    if [ -n "$TEST_STAGE" ]
    then
        # Run the tests with all the mods, without actually running any tests,
        # just to verify that all the mod data can be successfully loaded.
        # Because some mods might be mutually incompatible we might need to run a few times.

        ./build-scripts/get_all_mods.py | \
            while read mods
            do
                run_test ./tests/cata_test '(all_mods)=> ' '[force_load_game]' --user-dir=all_modded --mods="${mods}"
            done
    fi
fi

# vim:tw=0
