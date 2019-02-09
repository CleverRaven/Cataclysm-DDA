#!/bin/bash

# Features Content Interface Mods Balance Bugfixes Performance Infrastructure Build I18N

while read -r category description; do
    if [ -z "$category" ] || [[ "*" == "$category" ]] || [[ $( expr $category : "2018" ) == 4 ]] ; then
        continue
    fi
    category=${category,,}
    category=${category~}
    if [[ "$category" == "I18n" ]] ; then
        category="I18N and A11Y"
    fi
    found_category=""
    skipped=""
    inserted=""
    while read -r line; do
        if [ -z "$found_category" ]; then
            if [[ "$line" == "## $category:" ]] ; then
                found_category="true"
            fi
        elif [ -z "$inserted" ] && [ "$line" == "" ]; then
            if [ -z "$skipped" ]; then
                skipped="true"
            else
                inserted="true"
                echo "$description" >> tmpfile
            fi
        fi
        echo "$line" >> tmpfile;
    done < "$2"
    if [ -z "$found_category" ]; then
	echo "couldn't find category $category"
    fi
    if [ -z "$inserted" ]; then
	echo "couldn't insert $category $description"
    fi
    mv tmpfile $2
done < "$1"
