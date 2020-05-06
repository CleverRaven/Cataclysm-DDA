#!/usr/bin/env bash

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

# merge incoming translations for each language specified on the commandline
if [ $# -gt 0 ]
then
    for n in $@
    do
        if [ -f lang/incoming/${n}.po ]
        then
            if [ -f lang/po/${n}.po ]
            then
                echo "merging lang/incoming/${n}.po"
                msgcat -F --use-first lang/incoming/${n}.po lang/po/${n}.po -o lang/po/${n}.po && rm lang/incoming/${n}.po
            else
                echo "importing lang/incoming/${n}.po"
                mv lang/incoming/${n}.po lang/po/${n}.po
            fi
        fi
    done
# if nothing specified, merge all incoming translations
elif [ -d lang/incoming ]
then
    shopt -s nullglob # work as expected if nothing matches *.po
    for f in lang/incoming/*.po
    do
        n=`basename ${f} .po`
        if [ -f lang/po/${n}.po ]
        then
            echo "merging ${f}"
            msgcat -F --use-first ${f} lang/po/${n}.po -o lang/po/${n}.po && rm ${f}
        else
            echo "importing ${f}"
            mv ${f} lang/po/${n}.po
        fi
    done
fi

# merge lang/po/cataclysm-dda.pot with .po file for each specified language
if [ $# -gt 0 ]
then
    for n in $@
    do
        echo "updating lang/po/${n}.po"
        msgmerge --sort-by-file --no-fuzzy-matching lang/po/${n}.po lang/po/cataclysm-dda.pot | msgattrib --sort-by-file --no-obsolete -o lang/po/${n}.po
    done
# otherwise merge lang/po/cataclysm-dda.pot with all .po files in lang/po
else
    for f in lang/po/*.po
    do
        echo "updating $f"
        msgmerge --sort-by-file --no-fuzzy-matching $f lang/po/cataclysm-dda.pot | msgattrib --sort-by-file --no-obsolete -o $f
    done
fi
