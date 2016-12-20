#!/bin/sh
export PATH=/usr/bin:/bin:/usr/sbin:/sbin

V_SCRIPT_PATH=$(dirname "${0}")
cd "${V_SCRIPT_PATH}/../Resources/"

if [[ -f ../MacOS/cataclysm ]]; then
    V_SHELL_SCRIPT="export PATH=${PATH}; cd '${PWD}' && ../MacOS/cataclysm; exit"
    osascript -e "tell application \"Terminal\" to activate do script \"${V_SHELL_SCRIPT}\""
else
    ../MacOS/cataclysm-tiles
fi
