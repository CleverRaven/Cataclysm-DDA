#!/bin/sh
export PATH=/usr/bin:/bin:/usr/sbin:/sbin

V_SCRIPT_PATH=$(dirname "${0}")
cd "${V_SCRIPT_PATH}/../Resources/"

V_KERNEL_RELEASE=$(uname -r | cut -d. -f1)
if [[ "${V_KERNEL_RELEASE}" -ge 11 ]]; then
    K_LIBRARY_PATH=DYLD_LIBRARY_PATH
    K_FRAMEWORK_PATH=DYLD_FRAMEWORK_PATH
else
    K_LIBRARY_PATH=DYLD_FALLBACK_LIBRARY_PATH
    K_FRAMEWORK_PATH=DYLD_FALLBACK_FRAMEWORK_PATH
fi

if [[ -f cataclysm ]]; then
    V_SHELL_SCRIPT="export PATH=${PATH} ${K_LIBRARY_PATH}=. ${K_FRAMEWORK_PATH}=.; cd '${PWD}' && ./cataclysm; exit"
    osascript -e "tell application \"Terminal\" to activate do script \"${V_SHELL_SCRIPT}\""
else
    export ${K_LIBRARY_PATH}=. ${K_FRAMEWORK_PATH}=.
    ./cataclysm-tiles
fi
