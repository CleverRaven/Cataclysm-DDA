#!/usr/bin/env bash

# Sometimes PO files pulled from Transifex are not accepted by GNU gettext, for example
#   lang/po/hu.po:430257: 'msgid' and 'msgstr' entries do not both begin with '\n'
#   lang/po/hu.po:534682: 'msgid' and 'msgstr' entries do not both end with '\n'
#   lang/po/hu.po:534692: 'msgid' and 'msgstr' entries do not both end with '\n'
#
# This script tries to compile each updated PO file, and revert PO files that cannot be compiled by GNU gettext.
# So the invalid PO files won't fail Basic Build CI test and block merging the i18n update pull requests.

function discard_po() {
    echo "Discarding $1"
    rm "$1"
}

for i in lang/po/*.po; do
    msgfmt -f -o /dev/null "$i" || discard_po "$i"
done

