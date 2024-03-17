#!/bin/bash

# Script made specifically for compiling without running tests on GitHub Actions

echo "Using bash version $BASH_VERSION"
set -exo pipefail

num_jobs=3

# We might need binaries installed via pip, so ensure that our personal bin dir is on the PATH
export PATH=$HOME/.local/bin:$PATH

if [ -n "$CROSS_COMPILATION" ]
then
    "$CROSS_COMPILATION$COMPILER" --version
else
    $COMPILER --version
fi

if [ -n "$TEST_STAGE" ]
then
    build-scripts/validate_json.py
    make style-all-json-parallel RELEASE=1

    tools/dialogue_validator.py data/json/npcs/* data/json/npcs/*/* data/json/npcs/*/*/*

    tools/json_tools/generic_guns_validator.py

    tools/json_tools/gun_variant_validator.py -v -cin data/json

    # Also build chkjson (even though we're not using it), to catch any
    # compile errors there
    make -j "$num_jobs" chkjson
# Skip the rest of the run if this change is pure json and this job doesn't test any extra mods
elif [ -n "$JUST_JSON" -a -z "$MODS" ]
then
    echo "Early exit on just-json change"
    exit 0
fi

ccache --zero-stats
# Increase cache size because debug builds generate large object files
ccache -M 5G
ccache --show-stats --verbose

if [ "$CMAKE" = "1" ]
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
        ${COMPILER:+-DCMAKE_CXX_COMPILER=$COMPILER} \
        -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
        -DCMAKE_BUILD_TYPE="$build_type" \
        -DTILES=${TILES:-0} \
        -DSOUND=${SOUND:-0} \
        ..
    make -j$num_jobs
else
    make -j "$num_jobs" RELEASE=1 CCACHE=1 CROSS="$CROSS_COMPILATION" LINTJSON=0 FRAMEWORK=1 UNIVERSAL_BINARY=1

    # For CI on macOS, patch the test binary so it can find SDL2 libraries.
    if [[ ! -z "$OS" && "$OS" = "macos-12" ]]
    then
        file tests/cata_test
        install_name_tool -add_rpath $HOME/Library/Frameworks tests/cata_test
    fi
fi

# vim:tw=0
