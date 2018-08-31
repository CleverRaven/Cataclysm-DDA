#!/bin/bash
if [ "$#" -gt 2 ]
then
    echo "Making backup at $1.bak"
    sed -e $2'q' $1 >> temp.cpp
    sed -e '1,'$2'd;'$3'q' $1 > to_format.cpp
    astyle --options=.astylerc -n to_format.cpp
    cat to_format.cpp >> temp.cpp
    sed -e '1,'$3'd' $1 >> temp.cpp
    echo "Finished styling lines $2 through $3."
    cp temp.cpp $1
    rm temp.cpp to_format.cpp
fi
