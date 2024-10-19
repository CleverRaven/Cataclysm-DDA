#!/bin/bash

###  FIX HERE VERSION IF NECESSARY  ###
###  LINKS ARE AT THE END OF SCRIPT  ###
SDL2_URL=https://github.com/katemonster33/SDL.git
SDL2_branch=cdda_android
SDL2_image_URL=https://github.com/libsdl-org/SDL_image.git
SDL2_image_branch=release-2.8.2
SDL2_mixer_URL=https://github.com/libsdl-org/SDL_mixer.git
SDL2_mixer_branch=release-2.8.0
SDL2_ttf_URL=https://github.com/libsdl-org/SDL_ttf.git
SDL2_ttf_branch=release-2.22.0

DEPS_ZIP_PATH=$(dir $1)/

# Global Variables #
NDK_DIR="/home/katie/android-ndk-r26b/"
INSTALL_DIR=$(pwd)
API="26"

function fix_sdl_mk
{
    MK_ADDON=$'include $(CLEAR_VARS)\\\n'
    MK_ADDON+=$'LOCAL_MODULE := SDL2\\\n'
    MK_ADDON+=$'LOCAL_SRC_FILES := '"$(realpath ../deps/jni/SDL2/$ARCH)"$'/libSDL2.so\\\n'
    MK_ADDON+=$'LOCAL_EXPORT_C_INCLUDES += '"$(realpath ../deps/jni/SDL2/include)"$'\\\n'
    MK_ADDON+="include \$(PREBUILT_SHARED_LIBRARY)"

    if [[ -e tmp.mk ]]; then mv -f tmp.mk Android.mk; fi
    cp -fva Android.mk tmp.mk
    sed -e $'/(call my-dir)/a\\\n'"$MK_ADDON" Android.mk 1<> Android.mk
}

function build_proj
{
    cd $1

    if [[ ! $1 == SDL2 ]]; then
        fix_sdl_mk ;
    fi

    $NDK_DIR/ndk-build -C ./ \
        NDK_PROJECT_PATH=$NDK_DIR \
        APP_BUILD_SCRIPT=Android.mk \
        APP_PLATFORM=android-$API \
        APP_ABI=$ARCH $NDK_OPTIONS \
        APP_ALLOW_MISSING_DEPS=$2 \
        NDK_OUT=obj \
        NDK_LIBS_OUT=../deps/jni/$1/

    if [[ ! $1 == SDL2 ]]; then
        rm ../deps/jni/$1/$ARCH/libSDL2.so
    fi

    cd ..
}

function clone_proj
{
    if [[ ! -e "$1" ]] then
        git clone "$2" -b "$3" --depth=1 "$1";
    fi
}

#################################################################################

NDK=$NDK_DIR/ndk-build

if [[ ! -e "$NDK" ]]; then
    echo "Can not find ndk-build in $NDK"; 
    exit 1;
fi

mkdir -p build/deps
cd build/deps
rm -rf ./*
unzip ../../app/deps.zip #create deps folder
cd ..

clone_proj SDL2 $SDL2_URL $SDL2_branch

clone_proj SDL2_image $SDL2_image_URL $SDL2_image_branch

clone_proj SDL2_mixer $SDL2_mixer_URL $SDL2_mixer_branch

clone_proj SDL2_ttf $SDL2_ttf_URL $SDL2_ttf_branch

./SDL2_mixer/external/download.sh
./SDL2_ttf/external/download.sh

cp -f SDL2/include/*.h deps/jni/SDL2/include/
cp -f SDL2_image/include/*.h deps/jni/SDL2_image/
cp -f SDL2_mixer/include/*.h deps/jni/SDL2_mixer/
cp -f SDL2_ttf/SDL_ttf.h deps/jni/SDL2_ttf/

for ARCH in armeabi-v7a arm64-v8a x86 x86_64
do
    build_proj SDL2 false

    build_proj SDL2_image true

    build_proj SDL2_image true

    build_proj SDL2_mixer true

    build_proj SDL2_ttf true
done
cd deps
zip ../deps.zip jni/ -r
cd ../..


echo "******** DONE ********"
