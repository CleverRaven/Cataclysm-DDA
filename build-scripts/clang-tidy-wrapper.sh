#!/bin/bash

set -eu
set -o pipefail

build_dir=${CATA_BUILD_DIR:-build}
plugin=${build_dir}/tools/clang-tidy-plugin/libCataAnalyzerPlugin.so

if [ -f "$plugin" ]
then
    set -x
    LD_PRELOAD=$plugin ${CATA_CLANG_TIDY} --enable-check-profile --store-check-profile=clang-tidy-trace "$@"
else
    set -x
    ${CATA_CLANG_TIDY} "$@"
fi
