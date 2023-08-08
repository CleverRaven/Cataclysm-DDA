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

function merge_lang
{
    f="lang/incoming/${n}.po"
    o="lang/po/${n}.po"
    if [ -f ${o} ]
    then
        echo "merging ${f}"
        msgcat -F --use-first ${f} ${o} -o ${o} && rm ${f}
    else
        echo "importing ${f}"
        mv ${f} ${o}
    fi

    # merge lang/po/cataclysm-dda.pot with .po file
    echo "updating $o"
    msgmerge --sort-by-file --no-fuzzy-matching $o lang/po/cataclysm-dda.pot | msgattrib --sort-by-file --no-obsolete -o $o
}

# merge incoming translations for each language specified on the commandline
if [ $# -gt 0 ]
then
    for n in $@
    do
        if [ -f lang/incoming/${n}.po ]
        then
            merge_lang "${n}"
        fi
    done
# if nothing specified, merge all incoming translations
elif [ -d lang/incoming ]
then
    shopt -s nullglob # work as expected if nothing matches *.po
    for f in lang/incoming/*.po
    do
        n=`basename ${f} .po`
        merge_lang "${n}"
    done
fi
