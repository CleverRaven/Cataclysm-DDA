#!/bin/bash

###  FIX HERE VERSION IF NECESSARY  ###
###  LINKS ARE AT THE END OF SCRIPT  ###
SDL2_URL=https://github.com/katemonster33/SDL.git
SDL2_branch=cdda_android
SDL2_image_URL=https://github.com/libsdl-org/SDL_image.git
SDL2_image_branch=release-2.8.2
SDL2_mixer_URL=https://github.com/libsdl-org/SDL_mixer.git
SDL2_mixer_branch=release-2.8.2
SDL2_ttf_URL=https://github.com/libsdl-org/SDL_ttf.git
SDL2_ttf_branch=release-2.22.0

DEPS_ZIP_PATH=$(dir $1)/

# Global Variables #
NDK_DIR="/home/katie/android-ndk-r26b/"
NDK_OPTIONS="$NDK_OPTIONS"
INSTALL_DIR=$(pwd)
API="26"

function fix_sdl_mk
{
    MK_ADDON=$'include $(CLEAR_VARS)\\\n'
    MK_ADDON+=$'LOCAL_MODULE := SDL2\\\n'
    MK_ADDON+=$'LOCAL_SRC_FILES := '"$(realpath ../deps/SDL2/$ARCH)"$'/libSDL2.so\\\n'
    MK_ADDON+=$'LOCAL_EXPORT_C_INCLUDES += '"$(realpath ../SDL2/include)"$'\\\n'
    MK_ADDON+="include \$(PREBUILT_SHARED_LIBRARY)"

    if [[ -e tmp.mk ]]; then mv -f tmp.mk Android.mk; fi
    cp -fva Android.mk tmp.mk
    sed -e $'/(call my-dir)/a\\\n'"$MK_ADDON" Android.mk 1<> Android.mk
}

#################################################################################


echo "Used \"NDK_OPTIONS\":\n$NDK_OPTIONS"

NDK=$NDK_DIR/ndk-build

if [[ -e "build" ]]; then 
    echo "build dir exists, nothing to do"
    exit 0; 
fi

if [[ ! -e "$NDK" ]]; then
    echo "Can not find ndk-build in $NDK"; 
    exit 1;
fi

mkdir build
cd build
unzip ../app/deps.zip #create deps folder

git clone $SDL2_URL -b $SDL2_branch --depth=1 SDL2

git clone $SDL2_image_URL -b $SDL2_image_branch --depth=1 SDL2_image

git clone $SDL2_mixer_URL -b $SDL2_mixer_branch --depth=1 SDL2_mixer
./SDL2_mixer/external/download.sh

git clone $SDL2_ttf_URL -b $SDL2_ttf_branch --depth=1 SDL2_ttf
./SDL2_ttf/external/download.sh

for ARCH in armeabi-v7a arm64-v8a x86 x86_64
do
    cd SDL2

    $NDK_DIR/ndk-build -C ./ NDK_PROJECT_PATH=$NDK_DIR \
        APP_BUILD_SCRIPT=Android.mk \
        APP_PLATFORM=android-$API APP_ABI=$ARCH $NDK_OPTIONS \
        NDK_OUT=obj NDK_LIBS_OUT=../deps/SDL2/

    cd ..
    
    cd SDL2_image

    fix_sdl_mk

	$NDK_DIR/ndk-build -C . NDK_PROJECT_PATH=$NDK_DIR APP_BUILD_SCRIPT=Android.mk \
        APP_PLATFORM=android-$API APP_ABI=$ARCH APP_ALLOW_MISSING_DEPS=true $NDK_OPTIONS \
        NDK_OUT=obj NDK_LIBS_OUT=../deps/SDL2_image

    cd ..

    cd SDL2_mixer
    fix_sdl_mk

	$NDK_DIR/ndk-build -C $DIR NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=Android.mk \
        APP_PLATFORM=android-$API APP_ABI=$ARCH APP_ALLOW_MISSING_DEPS=true $NDK_OPTIONS \
        NDK_OUT=$obj NDK_LIBS_OUT=../deps/SDL2_mixer

    cd ..

    cd SDL2_ttf
    fix_sdl_mk

	$NDK_DIR/ndk-build -C $DIR NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=Android.mk \
        APP_PLATFORM=android-$API APP_ABI=$ARCH APP_ALLOW_MISSING_DEPS=true $NDK_OPTIONS \
        NDK_OUT=$obj NDK_LIBS_OUT=../deps/SDL2_ttf

    cd ..
done
cd ..


echo "******** DONE ********"
