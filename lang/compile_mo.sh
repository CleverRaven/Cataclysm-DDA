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

# Check output dir here
# Backward compatibility
if [ -z $LOCALE_DIR ]
then
    LOCALE_DIR="lang/mo"
fi

# compile .mo file for each specified language
if [ $# -gt 0 ] && [ $1 != "all" ]
then
    for n in $@
    do
        f="lang/po/${n}.po"
        if [ $n = "en" ]; then
            # English is special: we do not actually need translation for English,
            # but due to a libintl bug (https://savannah.gnu.org/bugs/index.php?58006),
            # gettext would be extremely slow on MinGW targets if we do not compile
            # a .mo file.
            msgen lang/po/cataclysm-dda.pot --output-file=${f}
        fi
        mkdir -p $LOCALE_DIR/${n}/LC_MESSAGES
        msgfmt -f -o $LOCALE_DIR/${n}/LC_MESSAGES/cataclysm-dda.mo ${f}
    done
else
    # if nothing specified, compile .mo file for every .po file in lang/po
    # English is special: see comments above
    msgen lang/po/cataclysm-dda.pot --output-file=lang/po/en.po
    for f in lang/po/*.po
    do
        n=`basename $f .po`
        mkdir -p $LOCALE_DIR/${n}/LC_MESSAGES
        msgfmt -f -o $LOCALE_DIR/${n}/LC_MESSAGES/cataclysm-dda.mo ${f}
    done
fi
