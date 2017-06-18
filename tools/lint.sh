#!/bin/bash

PERL=`which perl`
if [ $? -ne 0 ]; then
    echo "perl is required to run this script"
    exit 69; # EX_UNAVAILABLE
fi

JQ=`which jq`
if [ $? -ne 0 ]; then
    echo "jq is required to run this script"
    exit 69; # EX_UNAVAILABLE
fi

LINTER=`dirname $(readlink -f "$0")`/format/format.pl
if [ ! -f $LINTER -o ! -r $LINTER ]; then
    echo "Missing linter"
    exit 70; # EX_SOFTWARE
fi

CONFIG=`dirname $(readlink -f "$0")`/../json_whitelist
if [ ! -f $CONFIG -o ! -r $CONFIG ]; then
    echo "Missing json_whitelist"
    exit 70; # EX_SOFTWARE
fi

for file in "$@"; do
    # check file exists and is readable
    if [ ! -f $file -o ! -r $file ]; then
        echo "File not found: $file"
        exit 66; # EX_NOINPUT
    fi

    # Check validity via JQ
    $JQ '.' $file >/dev/null;
    if [ $? -ne 0 ]; then
        echo "Syntax error in $file"
        exit 65; # EX_DATAERR
    fi

    # Lint to canonical format
    output=$($PERL $LINTER $file)
    if [ $? -ne 0 ]; then
        echo "Invalid definition in $file"
        exit 65; # EX_DATAERR
    fi

    # Update if not in canonical form
    echo "$output" | diff -q $file - > /dev/null
    if [ $? -ne 0 ]; then
        echo "Fixed JSON regressions in $file"
        echo "$output" > $file
    fi
done
