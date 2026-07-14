#!/usr/bin/env bash
# Ad-hoc codesign every nested Mach-O in a .app, then the bundle itself.
# CDDA places dylibs under Contents/Resources/ which codesign --deep skips;
# without per-file signing macOS rejects the bundle on launch.
#
# Usage: codesign-macos.sh <path/to/Cataclysm.app>

set -euo pipefail

APP_DIR="${1:?app bundle path required}"

if [[ ! -d "$APP_DIR" ]]; then
    echo "ERROR: $APP_DIR is not a directory" >&2
    exit 1
fi

if ! command -v codesign >/dev/null 2>&1; then
    echo "ERROR: codesign not available (not running on macOS?)" >&2
    exit 2
fi

echo "Signing nested Mach-O files in $APP_DIR"
# Find every Mach-O file under Contents/MacOS and Contents/Resources.
# Suffix matching alone misses helper executables; use file(1) probe.
while IFS= read -r -d '' candidate; do
    if file "$candidate" | grep -qE 'Mach-O'; then
        codesign --force --sign - "$candidate"
    fi
done < <(find "$APP_DIR/Contents/MacOS" "$APP_DIR/Contents/Resources" -type f \( -perm -u+x -o -name '*.dylib' \) -print0)

echo "Signing app bundle $APP_DIR"
codesign --force --sign - "$APP_DIR"

echo "Verifying signature integrity"
codesign --verify --deep --strict --verbose=2 "$APP_DIR"

echo "Codesign sequence complete for $APP_DIR"
