#!/bin/sh
export PATH=/usr/bin:/bin:/usr/sbin:/sbin

V_SCRIPT_PATH=$(dirname "${0}")

if [[ -f ${V_SCRIPT_PATH}/cataclysm ]]; then
    V_SHELL_SCRIPT="export PATH=${PATH}; ${V_SCRIPT_PATH}/cataclysm; exit"
    osascript -e "tell application \"Terminal\" to activate do script \"${V_SHELL_SCRIPT}\""
else
    ${V_SCRIPT_PATH}/cataclysm-tiles
fi
