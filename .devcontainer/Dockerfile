FROM mcr.microsoft.com/devcontainers/cpp:0-ubuntu-22.04

# NOTE: Uninstall CMake extensions
# This is not a supported method by CDDA maintainers and it's confusing have this in the editor
# TODO: Not possible yet, waiting on upstream. 
# Considered trying to get cheeky and deleting the folder but I believe that is held on the host
# This would require building out a more full featured container. Easier to wait and see. 
# RUN code --uninstall-extension ms-vscode.cmake-tools && \
#    code --uninstall-extensiontwxs.cmake

ARG REINSTALL_CMAKE_VERSION_FROM_SOURCE="3.22.2"

# Optionally install the cmake for vcpkg
COPY ./reinstall-cmake.sh /tmp/

RUN if [ "${REINSTALL_CMAKE_VERSION_FROM_SOURCE}" != "none" ]; then \
    chmod +x /tmp/reinstall-cmake.sh && /tmp/reinstall-cmake.sh ${REINSTALL_CMAKE_VERSION_FROM_SOURCE}; \
    fi \
    && rm -f /tmp/reinstall-cmake.sh

# [Optional] Uncomment this section to install additional vcpkg ports.
# RUN su vscode -c "${VCPKG_ROOT}/vcpkg install <your-port-name-here>"

# [Optional] Uncomment this section to install additional packages.
RUN apt-get update && export DEBIAN_FRONTEND=noninteractive \
    && apt-get -y install --no-install-recommends libsdl2-dev libsdl2-ttf-dev \
    libsdl2-image-dev libsdl2-mixer-dev libfreetype6-dev build-essential astyle ccache



# [Optional] uncomment this section to include dependencies for Windows cross-compilation from Linux
# This section should align with doc/COMPILING/COMPILING.md#cross-compile-to-windows-from-linux
# With the exception of the installation of packages that are already installed earlier in the Dockerfile
# Like: astyle, cmake
# RUN apt-get -y install autoconf automake autopoint bash bison bzip2 flex gettext git g++ \
#     gperf intltool libffi-dev libgdk-pixbuf2.0-dev libtool libltdl-dev libssl-dev \
#     libxml-parser-perl lzip make mingw-w64 openssl p7zip-full patch perl pkg-config \
#     python3 ruby scons sed unzip wget xz-utils g++-multilib libc6-dev-i386 libtool-bin python3-mako
    
# WORKDIR /opt
# RUN mkdir -p mxe
# RUN mkdir -p libbacktrace
# RUN git clone https://github.com/mxe/mxe.git ./mxe
# WORKDIR /opt/mxe

# # because of https://github.com/mxe/mxe/issues/2659, we have to link python 3 to python
# RUN ln /usr/bin/python3 /usr/bin/python
# # Builds windows version of SDL2 which is required to later cross-compile CDDA.
# RUN make -j4 MXE_TARGETS='x86_64-w64-mingw32.static i686-w64-mingw32.static' \
#     MXE_PLUGIN_DIRS=plugins/gcc12 sdl2 sdl2_ttf sdl2_image sdl2_mixer gettext
# WORKDIR /opt/libbacktrace
# RUN wget https://github.com/Qrox/libbacktrace/releases/download/2020-01-03/libbacktrace-x86_64-w64-mingw32.tar.gz
# RUN wget https://github.com/Qrox/libbacktrace/releases/download/2020-01-03/libbacktrace-i686-w64-mingw32.tar.gz
# RUN tar -xzf libbacktrace-x86_64-w64-mingw32.tar.gz --exclude=LICENSE -C /opt/mxe/usr/x86_64-w64-mingw32.static
# RUN tar -xzf libbacktrace-i686-w64-mingw32.tar.gz --exclude=LICENSE -C /opt/mxe/usr/i686-w64-mingw32.static
