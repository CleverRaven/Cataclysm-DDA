#!/bin/bash

git diff -U0 master | grep -E '\+\+\+' | grep -Eo 'src/\D+' | while read line
do
#    ./linediff.sh $line
    diff $line $line.bak
done
