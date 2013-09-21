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

# compile .mo file for each specified language
if [ $# -gt 0 ]
then
    for n in $@
    do
        f="lang/po/${n}.po"
        mkdir -p lang/mo/${n}/LC_MESSAGES
        msgfmt -f -o lang/mo/${n}/LC_MESSAGES/cataclysm-dda.mo ${f}
    done
else
# if nothing specified, compile .mo file for every .po file in lang/po
    for f in lang/po/*.po
    do
        n=`basename $f .po`
        mkdir -p lang/mo/${n}/LC_MESSAGES
        msgfmt -f -o lang/mo/${n}/LC_MESSAGES/cataclysm-dda.mo ${f}
    done
fi

cd $oldpwd

