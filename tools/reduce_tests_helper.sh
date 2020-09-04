#!/bin/bash

# Helper script for reduce_tests.sh

set -uex

# Disallow empty test list
if [ ! -s "$multidelta_all_files" ]
then
    exit 1
fi

if "$REDUCE_TESTS_TEST_EXE" -d yes --abort --rng-seed "$rng_seed" \
    $REDUCE_TESTS_EXTRA_OPTS \
    -f <(tr -d '{}' < "$multidelta_all_files")
then
    exit 1
fi

exit 0
