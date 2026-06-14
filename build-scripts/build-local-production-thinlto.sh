#!/usr/bin/env bash
set -euo pipefail

usage()
{
    cat <<'EOF'
Usage: build-scripts/build-local-production-thinlto.sh [options] [make-target-or-var ...]

Configures a local Cataclysm production Make build with Clang ThinLTO, -O3,
and CPU tuning for this host's AMD Zen 1 Threadripper 1920X.

Environment overrides:
  CATA_MEM_PER_JOB_MIB   Memory budget per make job. Default: 512
  CATA_RAM_TARGET_PERCENT
                           Max total RAM utilization target. Default: 90
  CATA_MIN_JOBS          Minimum computed make jobs. Default: 1
  CATA_MAX_JOBS          Upper cap after memory calculation. Default: 16
  CATA_MAKE_JOBS         Explicit job override, bypassing memory calculation.
  CATA_THINLTO_JOBS      ThinLTO backend jobs. Default: computed make jobs.
  CATA_CPU_FLAGS         CPU flags. Default: -march=znver1 -mtune=znver1
  CATA_BUILD_PREFIX      Output/object prefix. Default: local-o3-thinlto-
  CATA_LANGUAGES         Localization languages. Default: all
  CATA_CCACHE_DIR        ccache directory. Default: ./ccache
  CATA_FETCH_TILESETS    Fetch production tileset artifacts before make.
                         Default: 0

Examples:
  build-scripts/build-local-production-thinlto.sh --print-command
  build-scripts/build-local-production-thinlto.sh --fetch-tilesets
  CATA_MEM_PER_JOB_MIB=768 build-scripts/build-local-production-thinlto.sh
  CATA_BUILD_PREFIX= CATA_LANGUAGES= build-scripts/build-local-production-thinlto.sh bindist
EOF
}

die()
{
    echo "error: $*" >&2
    exit 2
}

positive_int()
{
    local name=$1
    local value=$2

    [[ $value =~ ^[1-9][0-9]*$ ]] || die "$name must be a positive integer, got '$value'"
}

non_negative_int()
{
    local name=$1
    local value=$2

    [[ $value =~ ^[0-9]+$ ]] || die "$name must be a non-negative integer, got '$value'"
}

cpu_count()
{
    nproc 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1
}

mem_available_mib()
{
    if [[ -r /proc/meminfo ]]; then
        awk '
            $1 == "MemAvailable:" {
                print int( $2 / 1024 )
                found = 1
                exit
            }
            END {
                if( !found ) {
                    exit 1
                }
            }
        ' /proc/meminfo
    else
        die "cannot determine available memory on this platform"
    fi
}

mem_total_mib()
{
    if [[ -r /proc/meminfo ]]; then
        awk '
            $1 == "MemTotal:" {
                print int( $2 / 1024 )
                found = 1
                exit
            }
            END {
                if( !found ) {
                    exit 1
                }
            }
        ' /proc/meminfo
    else
        die "cannot determine total memory on this platform"
    fi
}

memory_limited_jobs()
{
    local total_mib=$1
    local available_mib=$2
    local cpu_jobs=$3
    local mem_per_job_mib=$4
    local target_percent=$5
    local min_jobs=$6
    local max_jobs=$7
    local used_mib=$(( total_mib - available_mib ))
    local target_used_mib=$(( total_mib * target_percent / 100 ))
    local usable_mib=$(( target_used_mib - used_mib ))
    local jobs=1

    if (( usable_mib < 0 )); then
        usable_mib=0
    fi

    jobs=$(( usable_mib / mem_per_job_mib ))
    if (( jobs < min_jobs )); then
        jobs=$min_jobs
    fi
    if [[ -n $max_jobs ]] && (( jobs > max_jobs )); then
        jobs=$max_jobs
    fi
    if (( jobs > cpu_jobs )); then
        jobs=$cpu_jobs
    fi
    if (( jobs < 1 )); then
        jobs=1
    fi

    echo "$jobs"
}

quote_command()
{
    printf '%q ' "$@"
    printf '\n'
}

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd -- "$script_dir/.." && pwd)
overlay_mk="$repo_root/build-scripts/local-production-thinlto.mk"

print_command=0
fetch_tilesets=${CATA_FETCH_TILESETS:-0}
make_args=()
while (($#)); do
    case $1 in
        --help|-h)
            usage
            exit 0
            ;;
        --print-command)
            print_command=1
            ;;
        --fetch-tilesets)
            fetch_tilesets=1
            ;;
        --skip-tilesets)
            fetch_tilesets=0
            ;;
        *)
            make_args+=("$1")
            ;;
    esac
    shift
done

[[ -r $repo_root/Makefile ]] || die "cannot find Makefile in $repo_root"
[[ -r $overlay_mk ]] || die "cannot find $overlay_mk"
command -v make >/dev/null || die "make is required"
command -v clang++ >/dev/null || die "clang++ is required for ThinLTO"
command -v ld.lld >/dev/null || die "ld.lld is required for ThinLTO linking"
command -v llvm-ar >/dev/null || die "llvm-ar is required for ThinLTO archives"

mem_per_job_mib=${CATA_MEM_PER_JOB_MIB:-512}
target_percent=${CATA_RAM_TARGET_PERCENT:-90}
min_jobs=${CATA_MIN_JOBS:-1}
max_jobs=${CATA_MAX_JOBS:-16}
cpu_flags=${CATA_CPU_FLAGS:--march=znver1 -mtune=znver1}
build_prefix=${CATA_BUILD_PREFIX-local-o3-thinlto-}
languages=${CATA_LANGUAGES-all}
ccache_dir=${CATA_CCACHE_DIR:-$repo_root/ccache}

positive_int CATA_MEM_PER_JOB_MIB "$mem_per_job_mib"
positive_int CATA_RAM_TARGET_PERCENT "$target_percent"
positive_int CATA_MIN_JOBS "$min_jobs"
if [[ -n $max_jobs ]]; then
    positive_int CATA_MAX_JOBS "$max_jobs"
fi
if (( target_percent > 100 )); then
    die "CATA_RAM_TARGET_PERCENT must be at most 100, got '$target_percent'"
fi
[[ $fetch_tilesets =~ ^[01]$ ]] || die "CATA_FETCH_TILESETS must be 0 or 1"
mkdir -p "$ccache_dir"
export CCACHE_DIR=$ccache_dir

total_mib=$(mem_total_mib)
available_mib=$(mem_available_mib)
cpu_jobs=$(cpu_count)
positive_int cpu_count "$cpu_jobs"

if [[ -n ${CATA_MAKE_JOBS:-} ]]; then
    positive_int CATA_MAKE_JOBS "$CATA_MAKE_JOBS"
    make_jobs=$CATA_MAKE_JOBS
else
    make_jobs=$(memory_limited_jobs "$total_mib" "$available_mib" "$cpu_jobs" "$mem_per_job_mib" "$target_percent" "$min_jobs" "$max_jobs")
fi

thinlto_jobs=${CATA_THINLTO_JOBS:-$make_jobs}
positive_int CATA_THINLTO_JOBS "$thinlto_jobs"

# Keep the main Makefile's LTO switch off here: for Clang, LTO=1 appends
# full -flto. The overlay supplies -flto=thin and the matching link flags.
cmd=(
    make
    -f "$repo_root/Makefile"
    -f "$overlay_mk"
    --jobs="$make_jobs"
    CLANG=1
    RELEASE=1
    NATIVE=linux64
    TILES=1
    SOUND=1
    LOCALIZE=1
    LANGUAGES="$languages"
    USE_HOME_DIR=1
    BACKTRACE=1
    LIBBACKTRACE=1
    CCACHE=1
    TESTS=0
    RUNTESTS=0
    ASTYLE=0
    LINTJSON=0
    GOLD=0
    MOLD=0
    LTO=0
    THIN_AR=1
    BUILD_PREFIX="$build_prefix"
    LOCAL_CPU_FLAGS="$cpu_flags"
    LOCAL_THINLTO_JOBS="$thinlto_jobs"
)
fetch_cmd=( "$repo_root/build-scripts/fetch-production-tilesets.sh" )

cmd+=("${make_args[@]}")

echo "available memory: ${available_mib} MiB"
echo "total memory: ${total_mib} MiB"
echo "cpu threads: ${cpu_jobs}"
echo "make jobs: ${make_jobs} (${mem_per_job_mib} MiB/job, ${target_percent}% RAM target, max ${max_jobs})"
echo "ThinLTO jobs: ${thinlto_jobs}"
echo "CPU flags: ${cpu_flags}"
echo "ccache dir: ${CCACHE_DIR}"
echo "fetch production tilesets: ${fetch_tilesets}"

if (( print_command )); then
    if (( fetch_tilesets )); then
        quote_command "${fetch_cmd[@]}"
    fi
    quote_command "${cmd[@]}"
    exit 0
fi

cd "$repo_root"
if (( fetch_tilesets )); then
    "${fetch_cmd[@]}"
fi
exec "${cmd[@]}"
