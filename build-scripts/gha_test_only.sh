#!/bin/bash

# Script made specifically for running tests on GitHub Actions

echo "Using bash version $BASH_VERSION"
set -exo pipefail

num_jobs=3
parallel_opts="--verbose --linebuffer"
cata_test_opts="--min-duration 20 --use-colour yes --rng-seed time ${EXTRA_TEST_OPTS}"
[ -z $NUM_TEST_JOBS ] && num_test_jobs=3 || num_test_jobs=$NUM_TEST_JOBS

# We might need binaries installed via pip, so ensure that our personal bin dir is on the PATH
export PATH=$HOME/.local/bin:$PATH
# export so run_test can read it when executed by parallel
export cata_test_opts 

function run_test
{
    set -eo pipefail
    test_exit_code=0 sed_exit_code=0 exit_code=0
    test_bin=$1
    prefix=$2
    shift 2

    $WINE "$test_bin" ${cata_test_opts} "$@" 2>&1 | sed -E 's/^(::(warning|error|debug)[^:]*::)?/\1'"$prefix"'/' || test_exit_code="${PIPESTATUS[0]}" sed_exit_code="${PIPESTATUS[1]}"
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

    # Run regular tests
    [ -f "${bin_path}cata_test" ] && parallel ${parallel_opts} "run_test $(printf %q "${bin_path}")'/cata_test' '('{}')=> ' --user-dir=test_user_dir_{#} {}" ::: "[slow] ~starting_items" "~[slow] ~[.],starting_items"
    [ -f "${bin_path}cata_test-tiles" ] && parallel ${parallel_opts} "run_test $(printf %q "${bin_path}")'/cata_test-tiles' '('{}')=> ' --user-dir=test_user_dir_{#} {}" ::: "[slow] ~starting_items" "~[slow] ~[.],starting_items"
else
    export ASAN_OPTIONS=detect_odr_violation=1
    export UBSAN_OPTIONS=print_stacktrace=1
    parallel -j "$num_test_jobs" ${parallel_opts} "run_test './tests/cata_test' '('{}')=> ' --user-dir=test_user_dir_{#} {}" ::: "[slow] ~starting_items" "~[slow] ~[.],starting_items"
    if [ -n "$MODS" ]
    then
        parallel -j "$num_test_jobs" ${parallel_opts} "run_test './tests/cata_test' 'Mods-('{}')=> ' $(printf %q "${MODS}") --user-dir=modded_{#} {}" ::: "[slow] ~starting_items" "~[slow] ~[.],starting_items"
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
