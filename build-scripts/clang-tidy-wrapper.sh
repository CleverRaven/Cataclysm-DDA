#!/bin/bash

set -eu
set -o pipefail

plugin=build/tools/clang-tidy-plugin/libCataAnalyzerPlugin.so

set -x
if [ -f "$plugin" ]
then
    LD_PRELOAD=$plugin "$CATA_CLANG_TIDY" "$@"
else
    "$CATA_CLANG_TIDY" "$@"
fi
