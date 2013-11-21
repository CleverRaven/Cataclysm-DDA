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
if python lang/extract_json_strings.py 
then
    # update cataclysm-dda.pot
    xgettext --default-domain="cataclysm-dda" \
             --sort-by-file \
             --add-comments="~" \
             --output="lang/po/cataclysm-dda.pot" \
             --keyword="_" \
             --keyword="pgettext:1c,2" \
             src/*.cpp src/*.h lang/json/*.py
    # Fix msgfmt errors
    if [ "`head -n1 lang/po/cataclysm-dda.pot`" == "# SOME DESCRIPTIVE TITLE." ]
    then
        package="cataclysm-dda"
        version=$(grep '^VERSION *= *' Makefile | tr -d [:space:] | cut -f 2 -d '=')
        pot_file="lang/po/cataclysm-dda.pot"
        sed -e "1,6d" \
            -e "s/^\"Project-Id-Version:.*\"$/\"Project-Id-Version: $package $version\\\n\"/1" \
            -e "/\"Plural-Forms:.*\"$/d" $pot_file > $pot_file.temp
        mv $pot_file.temp $pot_file
    fi
else
    echo 'UPDATE ABORTED'
    cd $oldpwd
    exit 1
fi

# strip line-numbers from the .pot file
python lang/strip_line_numbers.py lang/po/cataclysm-dda.pot

cd $oldpwd

