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
