#!/bin/bash
set -exo pipefail

emsdk install 3.1.51
emsdk activate 3.1.51

make -j`nproc` NATIVE=emscripten BACKTRACE=0 TILES=1 TESTS=0 RUNTESTS=0 RELEASE=1 cataclysm-tiles.js
