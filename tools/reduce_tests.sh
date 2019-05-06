#!/bin/bash

# A helper script to use multidelta to reduce to a minimal set of tests to
# reproduce a failure.
# Sometimes the tests interact in unexpected ways, so you need to run two
# or three tests in order to cause the failure to occur.  It's tiresome to
# determine a minimal subset of tests by hand; this script automates it.

# I think https://github.com/DRMacIver/structureshrink would be better than
# multidelta for this; multidelta requires strange workarounds.  But
# multidelta is readily available in most distributions' packages, so using
# that.

set -ue

if ! which multidelta &>/dev/null
then
    echo "multidelta not found.  Please install it to use this script" >&2
    exit 1
fi

if [ $# != 1 ]
then
    printf "Usage: %s RNG_SEED\n" "$0" >&2
    exit 1
fi

if [ ! -x tests/cata_test ]
then
    printf "Please build cata_test and run this script from the top-level directory" >&2
    exit 1
fi

export rng_seed=$1

# For some reason the following cata_test command returns 140, rather than 0 (success).
# Not sure why.

# We have to add braces around the lines to avoid topformflat messing up the file.
# The braces are removed again inside our helper script.
./tests/cata_test --list-test-names-only '~[.]' | \
    grep '[^ ]' | sed 's/.*/{&}/' > list_of_tests || true
multidelta tools/reduce_tests_helper.sh list_of_tests

# vim:tw=0
