#!/bin/bash

# Script made specifically for running tests on GitHub Actions

echo "Using bash version $BASH_VERSION"
set -exo pipefail

num_jobs=$(nproc 2>/dev/null || echo 4)
parallel_opts="--verbose --linebuffer"
cata_test_opts="--min-duration 20 --use-colour yes --rng-seed time --order lex ${EXTRA_TEST_OPTS}"
[ -z "$NUM_TEST_JOBS" ] && num_test_jobs=$num_jobs || num_test_jobs=$NUM_TEST_JOBS

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

    # Keep the output prefix free of the shard path so it cannot break
    # run_test's sed expression.
    # Run regular tests
    if [ -f "${bin_path}cata_test" ]; then
        SHARDS=$(python3 build-scripts/shard_tests.py --bin "${bin_path}cata_test" --shards "$num_test_jobs")
        parallel -j "$num_test_jobs" ${parallel_opts} "run_test $(printf %q "${bin_path}")'/cata_test' '({#})=> ' --user-dir=test_user_dir_{#} -f {}" ::: $SHARDS
    fi
    if [ -f "${bin_path}cata_test-tiles" ]; then
        SHARDS=$(python3 build-scripts/shard_tests.py --bin "${bin_path}cata_test-tiles" --shards "$num_test_jobs")
        parallel -j "$num_test_jobs" ${parallel_opts} "run_test $(printf %q "${bin_path}")'/cata_test-tiles' '({#})=> ' --user-dir=test_user_dir_{#} -f {}" ::: $SHARDS
    fi
else
    export ASAN_OPTIONS=detect_odr_violation=1
    export UBSAN_OPTIONS=print_stacktrace=1
    SHARDS=$(python3 build-scripts/shard_tests.py --bin "./tests/cata_test" --shards "$num_test_jobs")
    parallel -j "$num_test_jobs" ${parallel_opts} "run_test './tests/cata_test' '({#})=> ' --user-dir=test_user_dir_{#} -f {}" ::: $SHARDS
    if [ -n "$MODS" ]
    then
        for MODSET in ${MODS//|/ }; do
            parallel -j "$num_test_jobs" ${parallel_opts} "run_test './tests/cata_test' 'Mods-({#})=> ' --mods=$(printf %q "${MODSET}") --user-dir=modded_{#} -f {}" ::: $SHARDS
        done
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
