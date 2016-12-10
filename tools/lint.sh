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

    key='' # default is unsorted

    IFS=$'\n' # read one line at a time excluding comments
    for line in $(awk '/^[^#]/' json_whitelist); do

        # expand any shell globs in json_whitelist
        for opt in $(echo $line | awk '{print $1}'); do

            # check if any expansions match current file
            if [ "$file" -ef $(readlink -f $opt) ]; then

                # if so then set sort key (if any)
                key=$(echo $line | awk '{print $2}')
            fi
        done
    done

    # Check validity via JQ
    $JQ '.' $file >/dev/null;
    if [ $? -ne 0 ]; then
        echo "Syntax error in $file"
        exit 65; # EX_DATAERR
    fi

    # Lint to canonical format, optionally sorting by key
    if [ -z $key ]; then
        output=$($PERL $LINTER $file)
    else
        output=$($JQ "sort_by(.$key)" $file | $PERL $LINTER)
    fi
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
