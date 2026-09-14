#!/bin/bash
#
# UNSUPPORTED: this build cannot currently succeed.
#
# The target was abandoned in 2025. The PR CI leg was commented out on
# 2025-07-14 in commit f530407bda (PR #81827), and the release "WebAssembly
# Bundle" matrix entry is commented out as well.
#
# SDL2 support has since been removed, so the SDL flags this script relied on
# are gone from the Makefile. Emscripten does ship an SDL3 port (3.4.2) and an
# SDL3_ttf port, but no SDL3_image or SDL3_mixer port, so how to build tiles
# here is an open question.
#
# The script is kept deliberately, in the state it bit-rotted into, for anyone
# who wants to revive the path.
#
set -exo pipefail

CCACHE=${CCACHE:-0}

emsdk install 3.1.51
emsdk activate 3.1.51
if [ "$CCACHE" == "1" ]
then
    emsdk activate ccache-git-emscripten-64bit
fi

make -j`nproc` NATIVE=emscripten BACKTRACE=0 TILES=1 TESTS=0 RUNTESTS=0 RELEASE=1 CCACHE="$CCACHE" LINTJSON=0 cataclysm-tiles.js
