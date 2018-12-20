#!/bin/bash

# Build script intended for use in Travis CI

set -ex

function run_tests
{
    if [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
        $WINE "$@" -r cata --rng-seed `gshuf -i 0-1000000000 -n 1`
    else
        $WINE "$@" -r cata --rng-seed `shuf -i 0-1000000000 -n 1`
    fi
}

if [ -n "$CMAKE" ]
then
    mkdir build
    cd build
    cmake \
        -DBACKTRACE=ON \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DTILES=${TILES:-0} \
        -DSOUND=${SOUND:-0} \
        ..
    make -j3
    cd ..
    #[ -f cata_test ] && run_tests ./cata_test
    #[ -f cata_test-tiles ] && run_tests ./cata_test-tiles
else
    make -j3 RELEASE=1 BACKTRACE=1 DEBUG_SYMBOLS=1 CROSS="$CROSS_COMPILATION"
    run_tests ./tests/cata_test
    if [ -n "$MODS" ]
    then
        run_tests ./tests/cata_test $MODS
    fi
    build-scripts/lint-json.sh
fi
