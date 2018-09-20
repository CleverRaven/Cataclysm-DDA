#!/bin/bash

CHANGELOG=../data/changelog.txt
CHANGELOG_NEW=./changelog.new
TYPE=$1
shift
MSG=$@
found_it=false

echo "Adding change "\"$MSG\"" to $TYPE"

if [[ ! -f $CHANGELOG ]]; then
    echo "Changelog not found.  Starting a new one at $CHANGELOG"
    echo -e "$TYPE:\n$MSG" >> $CHANGELOG
    exit
fi

rm -f $CHANGELOG_NEW
while read -r line; do
    echo "$line" >> $CHANGELOG_NEW

    if ! $found_it && [[ "$line" == "$TYPE:" ]]; then
        echo $MSG >> $CHANGELOG_NEW
        found_it=true
    fi
done <$CHANGELOG

if ! $found_it; then
    echo "Adding catagory $TYPE"
    echo -e "\n$TYPE:" >> $CHANGELOG_NEW
    echo $MSG >> $CHANGELOG_NEW
fi

mv $CHANGELOG_NEW $CHANGELOG
