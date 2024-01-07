#!/bin/bash
set -exo pipefail

mkdir -p build/
cp \
  index.html \
  cataclysm-tiles.{data,js,wasm} \
  data/font/Terminus.ttf \
  build
cp data/cataicon.ico build/favicon.ico
