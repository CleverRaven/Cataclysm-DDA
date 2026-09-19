#!/usr/bin/env bash
# Bundle the SDL3 ldd closure into a Linux bindist's lib/.
# Usage: bundle-sdl3-linux.sh <bindist_dir> <main_binary>
# Copies non-platform deps (with SONAME symlinks), RUNPATH=$ORIGIN each.

set -euo pipefail

BINDIST_DIR="${1:?bindist dir required}"
MAIN_BIN="${2:?main binary required}"
LIB_DIR="$BINDIST_DIR/lib"

mkdir -p "$LIB_DIR"

# Platform library name patterns. ldd entries matching any of these are
# treated as host-owned: not copied, walking stops.
PLATFORM_PATTERNS=(
    # libc base
    'libc\.so'
    'libc\.so\.6'
    'libdl\.so'
    'libm\.so'
    'libpthread\.so'
    'libstdc\+\+\.so'
    'libgcc_s\.so'
    'librt\.so'
    'libresolv\.so'
    'ld-linux-.*\.so'
    'linux-vdso\.so'
    # X11
    'libX11\.so'
    'libxcb.*\.so'
    'libXext\.so'
    'libXcursor\.so'
    'libXi\.so'
    'libXfixes\.so'
    'libXrandr\.so'
    'libXrender\.so'
    'libXss\.so'
    'libXtst\.so'
    'libXau\.so'
    'libXdmcp\.so'
    'libICE\.so'
    'libSM\.so'
    # Wayland / input
    'libwayland-.*\.so'
    'libxkbcommon.*\.so'
    'libdecor.*\.so'
    'libdbus-1\.so'
    'libibus-1\.0\.so'
    # Display / DRM
    'libdrm\.so'
    'libgbm\.so'
    'libudev\.so'
    'libglapi\.so'
    # GL / Vulkan
    'libGL\.so'
    'libGLX\.so'
    'libGLdispatch\.so'
    'libOpenGL\.so'
    'libEGL\.so'
    'libvulkan\.so'
    # Audio
    'libasound\.so'
    'libpulse\.so'
    'libpulsecommon-.*\.so'
    'libpipewire-.*\.so'
    'libspa-.*\.so'
    'libsystemd\.so'
    'libasyncns\.so'
    'libsndfile\.so'
    # Misc base
    'libffi\.so'
    'libbsd\.so'
    'libmd\.so'
)

is_platform() {
    local soname="$1"
    for pat in "${PLATFORM_PATTERNS[@]}"; do
        if [[ "$soname" =~ ^${pat} ]]; then
            return 0
        fi
    done
    return 1
}

# Read ldd output on stdin, emit "soname<TAB>realpath" per non-platform
# entry. Pathless entries (linux-vdso) are skipped.
collect_libs() {
    awk '
        /=> not found/ { next }
        / => / {
            soname = $1
            path = $3
            if (path == "" || path ~ /^\(/) next
            print soname "\t" path
        }
    '
}

declare -A COPIED
declare -A SEEN
declare -A ALIAS
# Map of copied real lib basename -> original resolved system path. Used for
# license attribution via dpkg -S after the walk completes.
declare -A REAL_TO_ORIGIN

ensure_alias() {
    local alias_name="$1" real_base="$2"
    if [[ -z "$alias_name" || "$alias_name" == "$real_base" ]]; then return; fi
    local key="$alias_name->$real_base"
    if [[ -n "${ALIAS[$key]:-}" ]]; then return; fi
    ALIAS[$key]=1
    ln -sf "$real_base" "$LIB_DIR/$alias_name"
}

walk_closure() {
    local target="$1"
    if [[ -n "${SEEN[$target]:-}" ]]; then return; fi
    SEEN[$target]=1

    local soname realpath_resolved real base_real base_path
    while IFS=$'\t' read -r soname realpath_resolved; do
        [[ -z "$soname" || -z "$realpath_resolved" ]] && continue
        if is_platform "$soname"; then
            continue
        fi
        # Resolve symlinks to the real file.
        real="$(readlink -f "$realpath_resolved")"
        base_real="$(basename "$real")"
        base_path="$(basename "$realpath_resolved")"

        # Always create the soname + resolved-path aliases (independent of
        # whether the real file was copied already on a prior alias).
        ensure_alias "$soname" "$base_real"
        ensure_alias "$base_path" "$base_real"

        if [[ -z "${COPIED[$real]:-}" ]]; then
            COPIED[$real]=1
            REAL_TO_ORIGIN[$base_real]="$real"
            cp -L "$real" "$LIB_DIR/$base_real"
            # Walk this library's own ldd closure.
            walk_closure "$real"
        fi
    done < <(ldd "$target" 2>/dev/null | collect_libs)
}

walk_closure "$MAIN_BIN"

# RUNPATH=$ORIGIN on each bundled lib: DT_RUNPATH does not inherit to
# grandchild loads, so each .so needs its own to find sibling peers.
if ! command -v patchelf >/dev/null 2>&1; then
    echo "patchelf not available; bundled libs will not resolve transitive deps" >&2
    exit 1
fi
for so in "$LIB_DIR"/*.so*; do
    if [[ -f "$so" && ! -L "$so" ]]; then
        # $ORIGIN is a literal dynamic-loader token, not a shell variable.
        # shellcheck disable=SC2016
        patchelf --set-rpath '$ORIGIN' "$so"
    fi
done

# The main binary is linked with an absolute RUNPATH to the build-time SDL3
# prefix as well as $ORIGIN/lib; pin it to the bundle alone so the shipped
# libs win and the build prefix never leaks into resolution.
# shellcheck disable=SC2016
patchelf --set-rpath '$ORIGIN/lib' "$MAIN_BIN"

# Gate: every bundled lib resolves with LD_LIBRARY_PATH unset, and the main
# binary resolves its SDL3 deps from bindist/lib/ not the host.
echo "Verifying main binary closure with LD_LIBRARY_PATH unset"
if env -u LD_LIBRARY_PATH ldd "$MAIN_BIN" 2>&1 | grep -E 'not found|=>\s*$' >&2; then
    echo "ERROR: main binary has unresolved libraries" >&2
    exit 2
fi
echo "Verifying each bundled lib closure"
for so in "$LIB_DIR"/*.so*; do
    if [[ -f "$so" && ! -L "$so" ]]; then
        if env -u LD_LIBRARY_PATH ldd "$so" 2>&1 | grep -E 'not found' >&2; then
            echo "ERROR: $so has unresolved libraries" >&2
            exit 3
        fi
    fi
done

# Assert each SDL3-family dep resolves under $LIB_DIR: catch a host SDL3
# winning rpath order over the bundle.
LIB_DIR_REAL="$(readlink -f "$LIB_DIR")"
while IFS= read -r line; do
    soname="$(awk '{print $1}' <<<"$line")"
    target="$(awk '{print $3}' <<<"$line")"
    [[ -z "$target" || "$target" == "(" ]] && continue
    case "$soname" in
        libSDL3*|libfreetype*|libharfbuzz*|libpng*|libjpeg*|libFLAC*|libmpg123*|libvorbis*|libogg*|libwavpack*)
            target_real="$(readlink -f "$target")"
            case "$target_real" in
                "$LIB_DIR_REAL"/*) ;;
                *)
                    echo "ERROR: $soname resolved to $target_real (outside $LIB_DIR_REAL)" >&2
                    exit 4
                    ;;
            esac
            ;;
    esac
done < <(env -u LD_LIBRARY_PATH ldd "$MAIN_BIN" 2>/dev/null | grep ' => ')

echo "SDL3 closure bundle complete in $LIB_DIR"

# Per-lib license: dpkg -S the ORIGINAL resolved path (the lib/ copy is not
# in any apt db). Source-built SDL3 libs use the staged LICENSE-SDL*.txt.
LICENSE_DIR="$BINDIST_DIR/licenses"
mkdir -p "$LICENSE_DIR"
declare -A LICENSED_PKGS
{
    echo "Bundled third-party libraries (bindist/lib/):"
    echo
} > "$LICENSE_DIR/README.txt"
for base_real in "${!REAL_TO_ORIGIN[@]}"; do
    origin="${REAL_TO_ORIGIN[$base_real]}"
    # Source-built SDL3 libs are not registered in apt; skip and rely on the
    # staged LICENSE-SDL*.txt at the bindist root.
    case "$base_real" in
        libSDL3*)
            echo "$base_real: covered by LICENSE-SDL.txt (libSDL3*_mixer also LICENSE-SDL3_mixer.txt)" >> "$LICENSE_DIR/README.txt"
            continue
            ;;
    esac
    # dpkg registers some base libs under the pre-usrmerge /lib path while
    # readlink -f yields /usr/lib (or vice versa). Try both before giving up.
    # dpkg -S exits non-zero on no match, so swallow it under set -e.
    pkg="$(dpkg -S "$origin" 2>/dev/null | head -n1 | cut -d: -f1)" || pkg=""
    if [[ -z "$pkg" ]]; then
        case "$origin" in
            /usr/lib/*) alt="${origin#/usr}" ;;
            /lib/*)     alt="/usr$origin" ;;
            *)          alt="" ;;
        esac
        if [[ -n "$alt" ]]; then
            pkg="$(dpkg -S "$alt" 2>/dev/null | head -n1 | cut -d: -f1)" || pkg=""
        fi
    fi
    if [[ -z "$pkg" ]]; then
        echo "$base_real: dpkg -S returned no owner; manual attribution required" >> "$LICENSE_DIR/README.txt"
        continue
    fi
    if [[ -n "${LICENSED_PKGS[$pkg]:-}" ]]; then
        echo "$base_real: covered by licenses/${pkg}.copyright" >> "$LICENSE_DIR/README.txt"
        continue
    fi
    LICENSED_PKGS[$pkg]=1
    src_copyright="/usr/share/doc/$pkg/copyright"
    if [[ -f "$src_copyright" ]]; then
        cp "$src_copyright" "$LICENSE_DIR/${pkg}.copyright"
        echo "$base_real: licenses/${pkg}.copyright" >> "$LICENSE_DIR/README.txt"
    else
        echo "$base_real: $src_copyright not found; manual attribution required" >> "$LICENSE_DIR/README.txt"
    fi
done

echo "License staging complete in $LICENSE_DIR"
