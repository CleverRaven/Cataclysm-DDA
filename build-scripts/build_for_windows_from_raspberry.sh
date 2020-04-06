#!/bin/bash -e

PLATFORM="x86_64-w64-mingw32.static"

# mxe directory and cataclysm repogitry is in same depth
make CROSS="../mxe/usr/bin/${PLATFORM}-" TILES=1 SOUND=1 RELEASE=1 LOCALIZE=1 LINTJSON=0 ASTYLE=0 RUNTESTS=0 BACKTRACE=0

