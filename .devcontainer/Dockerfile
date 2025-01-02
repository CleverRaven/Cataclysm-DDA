# Use the base image for C++ development
FROM mcr.microsoft.com/devcontainers/cpp:1-debian-12

# Set the CMake version to reinstall if needed
ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="none"

# Copy the script to reinstall CMake from source
COPY ./reinstall-cmake.sh /tmp/

# Reinstall CMake if a specific version is provided
RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
        chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE}; \
    fi \
    && rm -f /tmp/reinstall-cmake.sh

# Install additional system packages for debugging
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends \
       python3-pip \
       nodejs \
       npm \
       openjdk-17-jdk \
       gdb \
       valgrind \
       lsof \
       git \
       libsdl2-dev \
       libsdl2-ttf-dev \
       libharfbuzz-dev \
       libsdl2-image-dev \
       libjpeg-turbo8-dev \
       libtiff-dev \
       libwebp-dev \
       libzip-dev \
       libzstd-dev \
       libsdl2-mixer-dev \
       libflac-dev \
       libasound2-dev \
       libfreetype6-dev \
       build-essential \
       astyle \
       ccache \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Python setup
RUN python3 -m pip install --upgrade pip

# Node.js setup
RUN npm install -g yarn

# Install vcpkg if not already present
RUN git clone https://github.com/microsoft/vcpkg.git $VCPKG_INSTALLATION_ROOT \
    && cd $VCPKG_INSTALLATION_ROOT \
    && ./bootstrap-vcpkg.sh

# Copy project files into the container
COPY . /workspace

# Set the working directory in the container
WORKDIR /workspace

# Default command to launch a debugging shell
CMD ["bash"]
