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


echo "> Extracting strings from C++ code"
xgettext --default-domain="cataclysm-dda" \
         --add-comments="~" \
         --sort-by-file \
         --output="lang/po/base.pot" \
         --keyword="_" \
         --keyword="pgettext:1c,2" \
         --keyword="n_gettext:1,2" \
         --keyword="npgettext:1c,2,3" \
         --keyword="translate_marker" \
         --keyword="translate_marker_context:1c,2" \
         --keyword="to_translation:1,1t" \
         --keyword="to_translation:1c,2,2t" \
         --keyword="pl_translation:1,2,2t" \
         --keyword="pl_translation:1c,2,3,3t" \
         --from-code="UTF-8" \
         src/*.cpp src/*.h
if [ $? -ne 0 ]; then
    echo "Error in extracting strings from C++ code. Aborting."
    exit 1
fi

package="cataclysm-dda"
version=$(grep '^VERSION *= *' Makefile | tr -d [:space:] | cut -f 2 -d '=')

echo "> Extracting strings from JSON"
if ! lang/extract_json_strings.py \
        -i data \
        -i data/json \
        -i data/mods \
        -x data/mods/TEST_DATA \
        -X data/json/furniture_and_terrain/terrain-regional-pseudo.json \
        -X data/json/furniture_and_terrain/furniture-regional-pseudo.json \
        -X data/json/items/book/abstract.json \
        -X data/json/npcs/TALK_TEST.json \
        -X data/core/sentinels.json \
        -X data/raw/color_templates/no_bright_background.json \
        -X data/mods/Magiclysm/Spells/debug.json \
        -D data/mods/BlazeIndustries \
        -D data/mods/desert_region \
        -n "$package $version" \
        -r lang/po/base.pot
then
    echo "Error in extracting strings from JSON. Aborting."
    exit 1
fi

echo "> Unification of translation template"
msguniq -o lang/po/cataclysm-dda.pot lang/po/base.pot
if [ ! -f lang/po/cataclysm-dda.pot ]; then
    echo "Error in merging translation templates. Aborting."
    exit 1
fi
sed -i "/^#\. #-#-#-#-#  [a-zA-Z0-9(). -]*#-#-#-#-#$/d" lang/po/cataclysm-dda.pot

# convert line endings to unix
os="$(uname -s)"
if (! [ "${os##CYGWIN*}" ]) || (! [ "${os##MINGW*}" ])
then
    echo "> Converting line endings to Unix"
    if ! sed -i -e 's/\r$//' lang/po/cataclysm-dda.pot
    then
        echo "Line ending conversion failed. Aborting."
        exit 1
    fi
fi

# Final compilation check
echo "> Testing to compile translation template"
if ! msgfmt -c -o /dev/null lang/po/cataclysm-dda.pot
then
    echo "Translation template cannot be compiled. Aborting."
    exit 1
fi

# Check for broken Unicode symbols
echo "> Checking for wrong Unicode symbols"
if ! lang/unicode_check.py lang/po/cataclysm-dda.pot
then
    echo "Updated pot file contain broken Unicode symbols. Aborting."
    exit 1
fi

echo "ALL DONE!"
