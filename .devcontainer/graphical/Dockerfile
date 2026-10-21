FROM mcr.microsoft.com/devcontainers/cpp:1-ubuntu-24.04

# Ubuntu 24.04 does not package SDL3 >= 3.4.0, so the stack is built from
# source. Versions and CMake options mirror
# .github/actions/setup-sdl3-stack/action.yml; keep the two in step.
ARG SDL3_VERSION=3.4.10
ARG SDL3_IMAGE_VERSION=3.4.4
ARG SDL3_TTF_VERSION=3.2.2
ARG SDL3_MIXER_VERSION=3.2.4

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends \
    libncursesw5-dev gettext build-essential astyle ccache cmake pkg-config git \
    libflac-dev libogg-dev libvorbis-dev libmpg123-dev libwavpack-dev \
    libfreetype-dev libharfbuzz-dev libjpeg-dev libpng-dev libwebp-dev libtiff-dev \
    libasound2-dev libpulse-dev \
    libx11-dev libxext-dev libxcursor-dev libxi-dev libxfixes-dev libxrandr-dev \
    libxrender-dev libxss-dev libxtst-dev libxkbcommon-dev libwayland-dev libwayland-bin \
    wayland-protocols libdecor-0-dev libdrm-dev libgbm-dev libudev-dev \
    libegl-dev libgl-dev libgles-dev libvulkan-dev libpipewire-0.3-dev \
    libdbus-1-dev libibus-1.0-dev \
    && rm -rf /var/lib/apt/lists/*

RUN set -eux; \
    build_root="$(mktemp -d)"; cd "$build_root"; \
    git clone --depth 1 --branch "release-${SDL3_VERSION}" https://github.com/libsdl-org/SDL.git sdl3; \
    cmake -S sdl3 -B sdl3_build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON; \
    cmake --build sdl3_build -j"$(nproc)"; \
    cmake --install sdl3_build; \
    git clone --depth 1 --branch "release-${SDL3_IMAGE_VERSION}" https://github.com/libsdl-org/SDL_image.git sdl3_image; \
    cmake -S sdl3_image -B sdl3_image_build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON \
        -DSDLIMAGE_VENDORED=OFF -DSDLIMAGE_DEPS_SHARED=OFF \
        -DSDLIMAGE_PNG=ON -DSDLIMAGE_PNG_LIBPNG=ON -DSDLIMAGE_JPG=ON \
        -DSDLIMAGE_AVIF=OFF -DSDLIMAGE_JXL=OFF -DSDLIMAGE_TIF=OFF -DSDLIMAGE_WEBP=OFF; \
    cmake --build sdl3_image_build -j"$(nproc)"; \
    cmake --install sdl3_image_build; \
    git clone --depth 1 --branch "release-${SDL3_TTF_VERSION}" https://github.com/libsdl-org/SDL_ttf.git sdl3_ttf; \
    cmake -S sdl3_ttf -B sdl3_ttf_build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON \
        -DSDLTTF_VENDORED=OFF -DSDLTTF_PLUTOSVG=OFF; \
    cmake --build sdl3_ttf_build -j"$(nproc)"; \
    cmake --install sdl3_ttf_build; \
    git clone --depth 1 --branch "release-${SDL3_MIXER_VERSION}" https://github.com/libsdl-org/SDL_mixer.git sdl3_mixer; \
    cmake -S sdl3_mixer -B sdl3_mixer_build -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON \
        -DSDLMIXER_VENDORED=OFF -DSDLMIXER_DEPS_SHARED=OFF \
        -DSDLMIXER_FLAC=ON -DSDLMIXER_FLAC_LIBFLAC=ON -DSDLMIXER_FLAC_DRFLAC=OFF \
        -DSDLMIXER_MP3=ON -DSDLMIXER_MP3_MPG123=ON -DSDLMIXER_MP3_DRMP3=OFF \
        -DSDLMIXER_VORBIS_VORBISFILE=ON -DSDLMIXER_VORBIS_STB=OFF -DSDLMIXER_VORBIS_TREMOR=OFF \
        -DSDLMIXER_WAVPACK=ON \
        -DSDLMIXER_OPUS=OFF -DSDLMIXER_MOD=OFF -DSDLMIXER_MIDI=OFF -DSDLMIXER_GME=OFF; \
    cmake --build sdl3_mixer_build -j"$(nproc)"; \
    cmake --install sdl3_mixer_build; \
    ldconfig; \
    rm -rf "$build_root"
