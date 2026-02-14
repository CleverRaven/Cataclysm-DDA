#!/bin/bash

set -eu
set -o pipefail

plugin=build/tools/clang-tidy-plugin/libCataAnalyzerPlugin.so

profile_args=""
if [ "${CATA_CLANG_TIDY_PROFILE:-}" = "1" ]; then
    profile_args="--enable-check-profile --store-check-profile=clang-tidy-trace"
fi

if [ -f "$plugin" ]
then
    set -x
    if command -v clang-tidy-cache >/dev/null 2>&1; then
        LD_PRELOAD=$plugin clang-tidy-cache "${CATA_CLANG_TIDY}" $profile_args "$@"
    else
        LD_PRELOAD=$plugin "${CATA_CLANG_TIDY}" $profile_args "$@"
    fi
else
    set -x
    if command -v clang-tidy-cache >/dev/null 2>&1; then
        clang-tidy-cache "${CATA_CLANG_TIDY}" "$@"
    else
        "${CATA_CLANG_TIDY}" "$@"
    fi
fi
