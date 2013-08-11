#!/bin/bash

oldpwd=`pwd`

if [ ! -d lang/po ]
then
    if [ -d ../lang/po ]
    then
        cd ..
    else
        echo "Error: Could not find lang/po subdirectory."
        exit 1
    fi
fi

# extract translatable strings from .json files
python lang/extract_json_strings.py

# update cataclysm-dda.pot
xgettext -d cataclysm-dda -F -c~ -o lang/po/cataclysm-dda.pot --keyword=_ *.cpp *.h lang/json/*.py

cd $oldpwd

