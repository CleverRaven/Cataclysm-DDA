#!/bin/sh

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
echo "> Extracting strings from json"
if ! lang/extract_json_strings.py
then
    echo "Error in extract_json_strings.py. Aborting"
    exit 1
fi

# Update cataclysm-dda.pot
echo "> Running xgettext to create .pot file"
xgettext --default-domain="cataclysm-dda" \
         --add-comments="~" \
         --sort-by-file \
         --output="lang/po/cataclysm-dda.pot" \
         --keyword="_" \
         --keyword="pgettext:1c,2" \
         --keyword="ngettext:1,2" \
         --keyword="translate_marker" \
         --keyword="translate_marker_context:1c,2" \
         --from-code="UTF-8" \
         src/*.cpp src/*.h lang/json/*.py lang/extra/android/*.cpp
if [ $? -ne 0 ]; then
    echo "Error in xgettext. Aborting"
    exit 1
fi

# Fix msgfmt errors
if [ "`head -n1 lang/po/cataclysm-dda.pot`" = "# SOME DESCRIPTIVE TITLE." ]
then
    echo "> Fixing .pot file headers"
    package="cataclysm-dda"
    version=$(grep '^VERSION *= *' Makefile | tr -d [:space:] | cut -f 2 -d '=')
    pot_file="lang/po/cataclysm-dda.pot"
    sed -e "1,6d" \
    -e "s/^\"Project-Id-Version:.*\"$/\"Project-Id-Version: $package $version\\\n\"/1" \
    -e "/\"Plural-Forms:.*\"$/d" $pot_file > $pot_file.temp
    mv $pot_file.temp $pot_file
fi

# strip line-numbers from the .pot file
echo "> Stripping .pot file from unneeded comments"
if ! python lang/strip_line_numbers.py lang/po/cataclysm-dda.pot
then
    echo "Error in strip_line_numbers.py. Aborting"
    exit 1
fi

# Final compilation check
echo "> Testing to compile the .pot file"
if ! msgfmt -c -o /dev/null lang/po/cataclysm-dda.pot
then
    echo "Updated pot file contain gettext errors. Aborting."
    exit 1
fi

# Check for broken Unicode symbols
echo "> Checking for wrong Unicode symbols"
if ! python lang/unicode_check.py lang/po/cataclysm-dda.pot
then
    echo "Updated pot file contain broken Unicode symbols. Aborting."
    exit 1
fi

echo "ALL DONE!"
