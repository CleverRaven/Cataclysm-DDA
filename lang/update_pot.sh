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

# Update cataclysm-bn.pot
echo "> Running xgettext to create .pot file"
xgettext --default-domain="cataclysm-bn" \
         --add-comments="~" \
         --sort-by-file \
         --output="lang/po/cataclysm-bn.pot" \
         --keyword="_" \
         --keyword="pgettext:1c,2" \
         --keyword="vgettext:1,2" \
         --keyword="vpgettext:1c,2,3" \
         --keyword="translate_marker" \
         --keyword="translate_marker_context:1c,2" \
         --keyword="to_translation:1,1t" \
         --keyword="to_translation:1c,2,2t" \
         --keyword="pl_translation:1,2,2t" \
         --keyword="pl_translation:1c,2,3,3t" \
         --from-code="UTF-8" \
         src/*.cpp src/*.h lang/json/*.py
if [ $? -ne 0 ]; then
    echo "Error in xgettext. Aborting"
    exit 1
fi

# Fix msgfmt errors
if [ "`head -n1 lang/po/cataclysm-bn.pot`" = "# SOME DESCRIPTIVE TITLE." ]
then
    echo "> Fixing .pot file headers"
    package="cataclysm-bn"
    version=$(grep '^VERSION *= *' Makefile | tr -d [:space:] | cut -f 2 -d '=')
    pot_file="lang/po/cataclysm-bn.pot"
    sed -e "1,6d" \
    -e "s/^\"Project-Id-Version:.*\"$/\"Project-Id-Version: $package $version\\\n\"/1" \
    -e "/\"Plural-Forms:.*\"$/d" $pot_file > $pot_file.temp
    mv $pot_file.temp $pot_file
fi

# strip line-numbers from the .pot file
echo "> Stripping .pot file from unneeded comments"
if ! lang/strip_line_numbers.py lang/po/cataclysm-bn.pot
then
    echo "Error in strip_line_numbers.py. Aborting"
    exit 1
fi

# convert line endings to unix
if [[ $(uname -s) =~ ^\(CYGWIN|MINGW\)* ]]
then
    echo "> Converting line endings to Unix"
    if ! sed -i -e 's/\r$//' lang/po/cataclysm-bn.pot
    then
        echo "Line ending conversion failed. Aborting."
        exit 1
    fi
fi

# Final compilation check
echo "> Testing to compile the .pot file"
if ! msgfmt -c -o /dev/null lang/po/cataclysm-bn.pot
then
    echo "Updated pot file contain gettext errors. Aborting."
    exit 1
fi

# Check for broken Unicode symbols
echo "> Checking for wrong Unicode symbols"
if ! lang/unicode_check.py lang/po/cataclysm-bn.pot
then
    echo "Updated pot file contain broken Unicode symbols. Aborting."
    exit 1
fi

echo "ALL DONE!"
