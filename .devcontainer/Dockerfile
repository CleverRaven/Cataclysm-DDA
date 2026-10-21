FROM mcr.microsoft.com/devcontainers/cpp:1-ubuntu-24.04

# Curses builds work out of the box here. Tiles builds need SDL3 >= 3.4.0,
# which Ubuntu 24.04 does not package; use the graphical devcontainer, which
# builds the SDL3 stack from source, or follow the SDL3 section of
# doc/c++/COMPILING.md.
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends libncursesw5-dev gettext \
    build-essential astyle ccache cmake pkg-config \
    && rm -rf /var/lib/apt/lists/*
