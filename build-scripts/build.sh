#!/bin/bash

# Build script intended for use in Travis CI

set -ex

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
    ctest --output-on-failure
else
    make -j3 RELEASE=1 BACKTRACE=1 DEBUG_SYMBOLS=1 CROSS="$CROSS_COMPILATION"
    $WINE ./tests/cata_test -r cata --rng-seed `shuf -i 0-1000000000 -n 1`
    if [ -n "$MODS" ]
    then
        $WINE ./tests/cata_test -r cata --rng-seed `shuf -i 0-1000000000 -n 1` $MODS
    fi
fi
