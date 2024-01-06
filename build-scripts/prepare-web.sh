#!/bin/bash -eo pipefail -x

mkdir -p build/
cp index.html build/
cp cataclysm-tiles.data build/
cp cataclysm-tiles.js build/
cp cataclysm-tiles.wasm build/
cp cataclysm-tiles.wasm.debug.wasm build/
cp data/cataicon.ico build/favicon.ico
cp data/font/Terminus.ttf build/Terminus.ttf
