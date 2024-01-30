FROM mcr.microsoft.com/devcontainers/cpp:0-ubuntu-22.04

RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends libsdl2-dev libsdl2-ttf-dev libharfbuzz-dev \
    libsdl2-image-dev libjpeg-turbo8-dev libtiff-dev libwebp-dev libzip-dev libzstd-dev \
    libsdl2-mixer-dev libflac-dev libasound2-dev libfreetype6-dev build-essential astyle ccache

# [Optional] comment out this section unless you want to compile the Object Creator. Install some QT libraries
RUN apt-get -y install qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools
