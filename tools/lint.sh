#!/bin/sh

PERL=`which perl`
if [ $? -ne 0 ]; then
    echo "perl is required to run this script"
    exit 69; # EX_UNAVAILABLE
fi

LINTER=`dirname $(readlink -f "$0")`/format/format.pl
if [ ! -f $LINTER -o ! -r $LINTER ]; then
    echo "Missing linter"
    exit 70; # EX_SOFTWARE
fi

for file in "$@"; do
    echo -n "Linting $file: "
    $PERL $LINTER -cq $file || exit 65; # EX_DATAERR
    echo OK
done
