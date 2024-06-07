#!/bin/bash
set -exo pipefail

rm -rf web_bundle

BUNDLE_DIR=web_bundle
DATA_DIR=$BUNDLE_DIR/data
mkdir -p $DATA_DIR
cp -R data/{core,font,fontdata.json,json,mods,names,raw,motd,credits,title,help} $DATA_DIR/
cp -R gfx $BUNDLE_DIR/

# Remove .DS_Store files.
find web_bundle -name ".DS_Store" -type f -exec rm {} \;

# Remove obsolete mods.
echo "Removing obsolete mods..."
for MOD_DIR in $DATA_DIR/mods/*/ ; do
    if jq -e '.[] | select(.type == "MOD_INFO") | .obsolete' "$MOD_DIR/modinfo.json" >/dev/null; then
        echo "$MOD_DIR is obsolete, excluding from web_bundle..."
        rm -rf $MOD_DIR
    fi
done

echo "Removing MA mod..."
rm -rf $DATA_DIR/mods/MA
echo "Removing Ultica_iso tileset..."
rm -rf $BUNDLE_DIR/gfx/Ultica_iso

$EMSDK/upstream/emscripten/tools/file_packager cataclysm-tiles.data --js-output=cataclysm-tiles.data.js --no-node --preload "$BUNDLE_DIR""@/" --lz4

mkdir -p build/
cp \
  build-data/web/index.html \
  cataclysm-tiles.{data,data.js,js,wasm} \
  data/font/Terminus.ttf \
  build
cp data/cataicon.ico build/favicon.ico
