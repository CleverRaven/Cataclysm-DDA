#!/bin/bash
if [ "$#" -gt 0 ]
then
    echo "Making backup at $1.bak"
    cp $1 $1.bak
    EXTRA=$(echo 0)
    WC=$(wc -l $1 | grep -Eo '\d+')
    git diff -U0 master $1 | grep -E '@@' | grep -Eo '\+\d+,?\d+ @@' | grep -Eo '\d+,?\d+' | while read line
    do
        ALPHA=$(echo $line | awk -F"," '{print $1}')
        DELTA=$(echo $line | awk -F"," '{print $2}')
        ALPHA=$(($ALPHA+$EXTRA))
        if [ "$(($DELTA))" -lt 1 ]
        then
            DELTA=$(echo 1)
        fi
        DELTA=$(($DELTA-1))
        OMEGA=$(($ALPHA+$DELTA))
        sed -e '1,'$ALPHA'd;'$OMEGA'q' $1 > to_format.cpp
        astyle --options=.astylerc -n to_format.cpp
        sed -e $ALPHA'q' $1 > buffer
        cat to_format.cpp >> buffer
        sed -e '1,'$OMEGA'd' $1 >> buffer
        cp buffer $1

        WCL=$(wc -l $1 | grep -Eo '\d+')
        EXTRA=$(($WCL+$WC))
        echo "Finished styling lines $ALPHA through $OMEGA."
    done
    rm buffer to_format.cpp
#    diff $1 $1.bak
fi
