#!/usr/bin/env bash
set -euo pipefail

usage()
{
    cat <<'EOF'
Usage: build-scripts/fetch-production-tilesets.sh [options]

Fetches the same precomposed tileset artifacts used by the experimental release
workflow and merges them into ./gfx for local runs and local bindist builds.

Options:
  --repo OWNER/REPO       GitHub repository to read workflow artifacts from.
                          Default: CleverRaven/Cataclysm-DDA
  --workflow WORKFLOW     Workflow file/name used to find the latest run.
                          Default: release.yml
  --run-id ID             Download artifacts from a specific workflow run.
  --dest DIR              Destination gfx directory. Default: ./gfx
  --download-dir DIR      Reuse or populate an explicit artifact directory.
  --keep-download         Do not delete the temporary artifact download dir.
  --overwrite-tracked     Allow replacing tracked tileset directories.
  --help                  Show this help.

Environment overrides mirror the option names:
  CATA_TILESET_REPO
  CATA_TILESET_WORKFLOW
  CATA_TILESET_RUN_ID
  CATA_TILESET_DEST
  CATA_TILESET_DOWNLOAD_DIR
  CATA_KEEP_TILESET_DOWNLOAD
  CATA_OVERWRITE_TRACKED_TILESETS
  CATA_TILESET_ARTIFACT_PATTERN
EOF
}

die()
{
    echo "error: $*" >&2
    exit 2
}

repo_root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)

repo=${CATA_TILESET_REPO:-CleverRaven/Cataclysm-DDA}
workflow=${CATA_TILESET_WORKFLOW:-release.yml}
run_id=${CATA_TILESET_RUN_ID:-}
dest=${CATA_TILESET_DEST:-$repo_root/gfx}
download_dir=${CATA_TILESET_DOWNLOAD_DIR:-}
keep_download=${CATA_KEEP_TILESET_DOWNLOAD:-0}
overwrite_tracked=${CATA_OVERWRITE_TRACKED_TILESETS:-0}
artifact_pattern=${CATA_TILESET_ARTIFACT_PATTERN:-tileset-*}

while (($#)); do
    case $1 in
        --help|-h)
            usage
            exit 0
            ;;
        --repo)
            shift
            (($#)) || die "--repo requires a value"
            repo=$1
            ;;
        --workflow)
            shift
            (($#)) || die "--workflow requires a value"
            workflow=$1
            ;;
        --run-id)
            shift
            (($#)) || die "--run-id requires a value"
            run_id=$1
            ;;
        --dest)
            shift
            (($#)) || die "--dest requires a value"
            dest=$1
            ;;
        --download-dir)
            shift
            (($#)) || die "--download-dir requires a value"
            download_dir=$1
            keep_download=1
            ;;
        --keep-download)
            keep_download=1
            ;;
        --overwrite-tracked)
            overwrite_tracked=1
            ;;
        *)
            die "unknown option: $1"
            ;;
    esac
    shift
done

[[ $keep_download =~ ^[01]$ ]] || die "CATA_KEEP_TILESET_DOWNLOAD must be 0 or 1"
[[ $overwrite_tracked =~ ^[01]$ ]] || die "CATA_OVERWRITE_TRACKED_TILESETS must be 0 or 1"

command -v gh >/dev/null || die "gh is required to download release workflow artifacts"
command -v git >/dev/null || die "git is required to detect tracked gfx directories"

if [[ -z $run_id ]]; then
    run_id=$(
        gh run list \
            --repo "$repo" \
            --workflow "$workflow" \
            --status success \
            --limit 1 \
            --json databaseId \
            --jq '.[0].databaseId'
    )
    [[ -n $run_id && $run_id != null ]] || die "no successful run found for $repo workflow $workflow"
fi

if [[ -z $download_dir ]]; then
    download_dir=$(mktemp -d "${TMPDIR:-/tmp}/cdda-production-tilesets.XXXXXXXXXX")
    cleanup_download=1
else
    mkdir -p "$download_dir"
    cleanup_download=0
fi

cleanup()
{
    if (( cleanup_download && ! keep_download )); then
        rm -rf -- "$download_dir"
    fi
}
trap cleanup EXIT

mkdir -p "$dest"

echo "tileset artifact repo: $repo"
echo "tileset artifact workflow: $workflow"
echo "tileset artifact run: $run_id"
echo "tileset artifact pattern: $artifact_pattern"
echo "tileset destination: $dest"
echo "tileset download dir: $download_dir"

gh run download "$run_id" \
    --repo "$repo" \
    --pattern "$artifact_pattern" \
    --dir "$download_dir"

shopt -s nullglob
artifact_dirs=( "$download_dir"/$artifact_pattern )
if (( ${#artifact_dirs[@]} == 0 )); then
    die "no artifact directories matching $artifact_pattern were downloaded"
fi

installed=()
skipped=()

for artifact_dir in "${artifact_dirs[@]}"; do
    [[ -d $artifact_dir ]] || continue

    source_root=$artifact_dir
    if [[ -d $artifact_dir/gfx ]]; then
        source_root=$artifact_dir/gfx
    fi

    entries=( "$source_root"/* )
    if (( ${#entries[@]} == 0 )); then
        echo "warning: no tileset payload found in $artifact_dir" >&2
        continue
    fi

    for entry in "${entries[@]}"; do
        base=$(basename -- "$entry")
        rel_path="gfx/$base"

        tracked=0
        if git -C "$repo_root" ls-files -- "$rel_path" | grep -q .; then
            tracked=1
        fi

        if (( tracked && ! overwrite_tracked )); then
            skipped+=( "$base" )
            continue
        fi

        if [[ -e $dest/$base ]]; then
            rm -rf -- "$dest/$base"
        fi

        cp -a -- "$entry" "$dest/"
        installed+=( "$base" )
    done
done

if (( ${#installed[@]} == 0 )); then
    echo "installed tilesets: none"
else
    printf 'installed tilesets: %s\n' "${installed[*]}"
fi

if (( ${#skipped[@]} != 0 )); then
    printf 'skipped tracked tilesets: %s\n' "${skipped[*]}"
    echo "rerun with --overwrite-tracked to replace those tracked directories"
fi
