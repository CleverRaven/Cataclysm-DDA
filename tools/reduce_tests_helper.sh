#!/bin/bash

# Helper script for reduce_tests.sh

set -uex

# Disallow empty test list
if [ ! -s "$1" ]
then
    exit 1
fi

# return 0 iff the expected error is emitted

# If you have a test string to watch for, add it here and comment out the later check for test success.
#"$REDUCE_TESTS_TEST_EXE" -d yes --rng-seed "$rng_seed" \
#    $REDUCE_TESTS_EXTRA_OPTS \
#    -f <(tr -d '{}' < <(cat $1)) 2>&1 | tee -a reduce_tests.log | grep -q '../tests/monster_test.cpp:1008: FAILED:'


if "$REDUCE_TESTS_TEST_EXE" -d yes --abort --rng-seed "$rng_seed" \
    $REDUCE_TESTS_EXTRA_OPTS \
    -f <(tr -d '{}' < "$multidelta_all_files")
then
    exit 1
fi

exit 0
