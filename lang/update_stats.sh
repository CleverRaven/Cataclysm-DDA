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

for f in lang/po/*.po
do
    n=`basename ${f} .po`
    o="lang/po/${n}.po"
    echo "getting stats for ${n}"
    num_translated=$( \
        msgattrib --translated "${o}" 2>/dev/null | grep -c '^msgid')
    num_untranslated=$( \
        msgattrib --untranslated "${o}" 2>/dev/null | grep -c '^msgid')
    mkdir -p lang/stats
    printf '{"%s"sv, %d, %d},\n' \
        "${n}" "$((num_translated-1))" "$((num_untranslated-1))" \
        > lang/stats/${n}
done

cat lang/stats/* > src/lang_stats.inc
