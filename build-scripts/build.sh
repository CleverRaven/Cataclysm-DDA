#!/bin/bash

# Build script intended for use in Travis CI

set -ex

function run_tests
{
    $WINE "$@" -d yes -r cata --rng-seed time $EXTRA_TEST_OPTS
}

if [ -n "$CMAKE" ]
then
    if [ "$RELEASE" = "1" ]
    then
        build_type=MinSizeRel
    else
        build_type=Debug
    fi

    mkdir build
    cd build
    cmake \
        -DBACKTRACE=ON \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DCMAKE_BUILD_TYPE="$build_type" \
        -DTILES=${TILES:-0} \
        -DSOUND=${SOUND:-0} \
        ..
    make -j3
    cd ..
    [ -f cata_test ] && run_tests ./cata_test
    [ -f cata_test-tiles ] && run_tests ./cata_test-tiles
else
    make -j3 RELEASE=1 BACKTRACE=1 DEBUG_SYMBOLS=1 CROSS="$CROSS_COMPILATION"
    run_tests ./tests/cata_test
    if [ -n "$MODS" ]
    then
        run_tests ./tests/cata_test $MODS
    fi
    build-scripts/lint-json.sh
fi
