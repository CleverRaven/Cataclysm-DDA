#!/bin/bash

# Shell script intended for clang-tidy check.
#
# Env vars:
#   LLVM_VERSION       clang-tidy version (default matches CI)
#   LLVM_PREFIX        install root, when neither /usr/lib/llvm-${ver}
#                      nor /usr/lib/llvm/${ver} matches
#   CATA_BUILD_DIR     build directory (default: build)
#   CATA_CLANG_TIDY    clang-tidy executable name (default: clang-tidy)

echo "Using bash version $BASH_VERSION"
set -exo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo_root="$(dirname "$script_dir")"

num_jobs=${num_jobs:-3}
build_type=${build_type:-MinSizeRel}
build_dir=${CATA_BUILD_DIR:-build}

llvm_version=${LLVM_VERSION:-18}

# Locate the LLVM install. The two probed layouts cover apt.llvm.org and
# distros that use versioned slots; LLVM_PREFIX bypasses both.
if [ -n "${LLVM_PREFIX:-}" ]; then
    llvm_prefix="$LLVM_PREFIX"
elif [ -d "/usr/lib/llvm-${llvm_version}" ]; then
    llvm_prefix="/usr/lib/llvm-${llvm_version}"
elif [ -d "/usr/lib/llvm/${llvm_version}" ]; then
    llvm_prefix="/usr/lib/llvm/${llvm_version}"
else
    echo "Cannot locate LLVM ${llvm_version} install. Set LLVM_PREFIX explicitly." >&2
    exit 1
fi

llvm_libdir=
for cand in "${llvm_prefix}/lib/cmake/llvm" "${llvm_prefix}/lib64/cmake/llvm"; do
    if [ -d "$cand" ]; then
        llvm_libdir="$cand"
        break
    fi
done
if [ -z "$llvm_libdir" ]; then
    echo "Cannot find LLVM cmake config under ${llvm_prefix}." >&2
    exit 1
fi
clang_libdir="${llvm_libdir%/llvm}/clang"

cmake_extra_opts=()
cmake_extra_opts+=("-DCATA_CLANG_TIDY_PLUGIN=ON")
cmake_extra_opts+=("-DLLVM_DIR=${llvm_libdir}")
cmake_extra_opts+=("-DClang_DIR=${clang_libdir}")


mkdir -p "$build_dir"
build_dir="$(cd "$build_dir" && pwd)"
cd "$build_dir"
cmake \
    ${COMPILER:+-DCMAKE_CXX_COMPILER=$COMPILER} \
    -DCMAKE_BUILD_TYPE="$build_type" \
    -DLOCALIZE=OFF \
    "${cmake_extra_opts[@]}" \
    "$repo_root"


echo "Compiling clang-tidy plugin"
make -j"$num_jobs" CataAnalyzerPlugin
#export PATH=$PWD/tools/clang-tidy-plugin/clang-tidy-plugin-support/bin:$PATH
# add FileCheck and the matching clang-tidy to the search path
export PATH="${llvm_prefix}/bin:$PATH"
if ! which FileCheck
then
    echo "Missing FileCheck"
    exit 1
fi
CATA_CLANG_TIDY=${CATA_CLANG_TIDY:-clang-tidy}
# lit might be installed via pip, so ensure that our personal bin dir is on the PATH
export PATH=$HOME/.local/bin:$PATH
lit -v tools/clang-tidy-plugin/test
cd "$repo_root"

# show that it works

plugin_so="${build_dir}/tools/clang-tidy-plugin/libCataAnalyzerPlugin.so"
echo "version:"
LD_PRELOAD="$plugin_so" "${CATA_CLANG_TIDY}" --version
echo "all enabled checks:"
LD_PRELOAD="$plugin_so" "${CATA_CLANG_TIDY}" --list-checks
echo "cata-specific checks:"
LD_PRELOAD="$plugin_so" "${CATA_CLANG_TIDY}" --list-checks --checks="cata-*" | grep "cata"
