#!sh
PATH="${PATH}:/home/lr/src/mxe/usr/bin" make -j8 RELEASE=1 LOCALIZE=1 CROSS=i686-w64-mingw32.static-  SDL2GPU_PREFIX='/usr/local'
