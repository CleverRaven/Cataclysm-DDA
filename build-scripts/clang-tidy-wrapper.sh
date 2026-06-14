#!/bin/bash

set -eu
set -o pipefail

build_dir=${CATA_BUILD_DIR:-build}
plugin=${build_dir}/tools/clang-tidy-plugin/libCataAnalyzerPlugin.so

# Tolerate GCC-built compile_commands driving clang-tidy (silence unknown
# -W flags) and dodge the clang>=22 c2y-extension warning on Catch2
# __COUNTER__ macros (llvm/llvm-project#189645).
extra_args=(
    --extra-arg=-Wno-unknown-warning-option
    --extra-arg=-DCATCH_CONFIG_NO_COUNTER
)

if [ -f "$plugin" ]
then
    set -x
    LD_PRELOAD=$plugin ${CATA_CLANG_TIDY} --enable-check-profile --store-check-profile=clang-tidy-trace "${extra_args[@]}" "$@"
else
    set -x
    ${CATA_CLANG_TIDY} "${extra_args[@]}" "$@"
fi
