#!/bin/bash

set -ev

make -j3 RELEASE=1 BACKTRACE=1 DEBUG_SYMBOLS=1 CROSS="$CROSS_COMPILATION"
$WINE ./tests/cata_test -r cata --rng-seed `shuf -i 0-1000000000 -n 1`
if [ -n "$MODS" ]
then
    $WINE ./tests/cata_test -r cata --rng-seed `shuf -i 0-1000000000 -n 1` $MODS
fi
