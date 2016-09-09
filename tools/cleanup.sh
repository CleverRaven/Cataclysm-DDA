#!/bin/sh

usage() {
    echo "Usage: $0 sort [files]..."
    exit 64; # EX_USAGE
}

validate() {
    for file in "$@"; do
        if [ ! -f $file -o ! -r $file ]; then
            echo "File not found: $file"
            exit 66; # EX_NOINPUT
        fi

        $PERL $LINTER -q $file;
        if [ $? -ne 0 ]; then
            echo "Cannot parse file: $file"
            exit 65; # EX_DATAERR
        fi
    done
}

cleanup() {
    script=$1
    shift;

    validate $@;

    for file in "$@"; do
        # Check if file is already in canonical format...
        output=`$JQ $script $file | $PERL $LINTER`
         echo "$output" | diff -q $file - > /dev/null

         # ...if not replace with the canonical form
        if [ $? -ne 0 ]; then
            echo "Fixed JSON regressions in $file"
            echo "$output" > $file
        fi
    done

    exit 0;
}

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

task=$1;
shift;

case $task in
    none    ) cleanup "." $@;;
    item    ) cleanup "sort_by(.id)" $@;;
    mission ) cleanup "sort_by(.id)" $@;;
    recipe  ) cleanup "sort_by(.result)" $@;;
    uncraft ) cleanup "sort_by(.result)" $@;;

    * ) usage;;
esac
