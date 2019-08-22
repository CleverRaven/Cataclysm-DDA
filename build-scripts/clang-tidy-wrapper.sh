#!/bin/bash

# Wrapper script for clang tidy which is a no-op after some time limit to avoid
# hitting the Travis build timeout.

set -eu
set -o pipefail

seconds_since_build_start=$(($(date +%s) - $(cat build-start-time)))
time_limit=$((15*60)) # Stop 15 minutes after build started
printf "%s/%s seconds elapsed\n" "$seconds_since_build_start" "$time_limit"
if [ "$seconds_since_build_start" -gt "$time_limit" ]
then
    printf "Skipping clang-tidy %s due to time limit\n" "$*"
    exit 0
fi

plugin=build/tools/clang-tidy-plugin/libCataAnalyzerPlugin.so

set -x
if [ -f "$plugin" ]
then
    LD_PRELOAD=$plugin "$CATA_CLANG_TIDY" "$@"
else
    "$CATA_CLANG_TIDY" "$@"
fi
