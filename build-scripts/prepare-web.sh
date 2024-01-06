#!/bin/bash
set -exo pipefail

mkdir -p build/
cp \
  index.html \
  cataclysm-tiles.{data,js,wasm,wasm.debug.wasm} \
  data/font/Terminus.ttf \
  build
cp data/cataicon.ico build/favicon.ico
