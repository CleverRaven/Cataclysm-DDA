#!/bin/bash

# Build script intended for use in Travis CI

set -ex

function run_tests
{
    $WINE "$@" -d yes -r cata --rng-seed time $EXTRA_TEST_OPTS
}

function just_json
{
    for filename in $(git diff --name-only $TRAVIS_BRANCH)
    do
        if [[ ! "$filename" =~ .json$ ]]
        then
            echo "$filename is not json, triggering full build."
            return 1
        fi
    done
    echo "Only json files present, skipping full build."
    return 0
}

if [ -n "$TEST_STAGE" ]
then
    build-scripts/lint-json.sh
    make -j 5 style-json
elif just_json
then
    CODE_COVERAGE=""
    exit 0
fi

ccache --zero-stats
if [ -n "$CMAKE" ]
then
    bin_path="./"
    if [ "$RELEASE" = "1" ]
    then
        build_type=MinSizeRel
        bin_path="build/tests/"
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
    [ -f "${bin_path}cata_test" ] && run_tests "${bin_path}cata_test"
    [ -f "${bin_path}cata_test-tiles" ] && run_tests "${bin_path}cata_test-tiles"
else
    make -j3 RELEASE=1 CCACHE=1 BACKTRACE=1 CROSS="$CROSS_COMPILATION" LINTJSON=0
    run_tests ./tests/cata_test
    if [ -n "$MODS" ]
    then
        run_tests ./tests/cata_test $MODS
    fi
fi
ccache --show-stats
