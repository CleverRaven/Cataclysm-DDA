#!/bin/sh
PWD=`dirname "${0}"`
cd "${PWD}/../Resources/"

COMMAND='
OSREV=`uname -r | cut -d. -f1`;
if [ $OSREV -ge 11 ] ; then
   export DYLD_LIBRARY_PATH=./;
   export DYLD_FRAMEWORK_PATH=./;
else
   export DYLD_FALLBACK_LIBRARY_PATH=./;
   export DYLD_FALLBACK_FRAMEWORK_PATH=./;
fi;'

if [ -f "cataclysm" ] ; then
    osascript -e 'tell application "Terminal" to activate do script "cd '`pwd`';'"${COMMAND}"'./cataclysm;exit;"'
else
    eval "${COMMAND}"
    ./cataclysm-tiles
fi