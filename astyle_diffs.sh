#!/bin/bash

git diff -U0 master | grep -E '\+\+\+' | grep -Eo '/\D+' | while read line
do
    ./linediff line
done
