#!/bin/bash

set -eu
set -o pipefail

plugin=build/tools/clang-tidy-plugin/libCataAnalyzerPlugin.so

if [ -f "$plugin" ]
then
    LD_PRELOAD=$plugin "$CATA_CLANG_TIDY" --enable-check-profile --store-check-profile=clang-tidy-trace "$@"
else
    "$CATA_CLANG_TIDY" "$@"
fi
