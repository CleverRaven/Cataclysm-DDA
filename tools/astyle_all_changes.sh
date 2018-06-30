#!/bin/bash

git diff -U0 master | grep -E '\+\+\+' | grep -Eo 'src/\D+' | while read line
do
    echo "tools/astyle_file_changes.sh $line"
    tools/astyle_file_changes.sh $line
done
