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

# try to extract translatable strings from .json files
echo "Updating gettext .pot file"
echo -n "Extracting strings from json..."
if python lang/extract_json_strings.py 
then
    echo " OK"
    # update cataclysm-dda.pot
    echo -n "Running xgettext to create .pot file..."
    xgettext --default-domain="cataclysm-dda" \
             --sort-by-file \
             --add-comments="~" \
             --output="lang/po/cataclysm-dda.pot" \
             --keyword="_" \
             --keyword="pgettext:1c,2" \
             src/*.cpp src/*.h lang/json/*.py
    echo " OK"
    # Fix msgfmt errors
    if [ "`head -n1 lang/po/cataclysm-dda.pot`" == "# SOME DESCRIPTIVE TITLE." ]
    then
        echo -n "Fixing .pot file headers..."
        package="cataclysm-dda"
        version=$(grep '^VERSION *= *' Makefile | tr -d [:space:] | cut -f 2 -d '=')
        pot_file="lang/po/cataclysm-dda.pot"
        sed -e "1,6d" \
            -e "s/^\"Project-Id-Version:.*\"$/\"Project-Id-Version: $package $version\\\n\"/1" \
            -e "/\"Plural-Forms:.*\"$/d" $pot_file > $pot_file.temp
        mv $pot_file.temp $pot_file
        echo " OK"
    fi
else
    echo "ABORTED"
    cd $oldpwd
    exit 1
fi

# strip line-numbers from the .pot file
echo -n "Stripping .pot file from unneeded comments..."
if python lang/strip_line_numbers.py lang/po/cataclysm-dda.pot
then
    echo " OK"
else
    echo "ABORTED"
    cd $oldpwd
    exit 1
fi

cd $oldpwd
