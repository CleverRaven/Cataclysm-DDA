#!/bin/sh
PWD=`dirname "${0}"`
OSREV=`uname -r | cut -d. -f1`
if [ "$OSREV" -ge 11 ] ; then
   export DYLD_LIBRARY_PATH=${PWD}/../Resources/
   export DYLD_FRAMEWORK_PATH=${PWD}/../Resources/
else
   export DYLD_FALLBACK_LIBRARY_PATH=${PWD}/../Resources/
   export DYLD_FALLBACK_FRAMEWORK_PATH=${PWD}/../Resources/
fi
cd "${PWD}/../Resources/"; ./cataclysm-tiles
