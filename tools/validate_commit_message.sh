#!/bin/bash

# Checks the commits msgs in the range of commits travis is testing.
# Based heavily on
# https://github.com/JensRantil/angular.js/blob/ffe93bb368037049820ac05ef62f8cc7ed379d98/test-commit-msgs.sh

# Travis's docs are misleading.
# Check for either a commit or a range (which apparently isn't always a range) and fix as needed.
if [ "$#" -gt 0 ]; then
	RANGE=$1
elif [ "$TRAVIS_COMMIT_RANGE" != "" ]; then
	RANGE=$TRAVIS_COMMIT_RANGE
elif [ "$TRAVIS_COMMIT" != "" ]; then
	RANGE=$TRAVIS_COMMIT
fi

if [ "$RANGE" == "" ]; then
	echo "RANGE is empty!"
	exit 1
fi

# Travis sends the ranges with 3 dots. Git only wants 2.
if [[ "$RANGE" == *...* ]]; then
	RANGE=`echo $RANGE | sed 's/\.\.\./../'`
elif [[ "$RANGE" != *..* ]]; then
	RANGE="$RANGE~..$RANGE"
fi

echo "$RANGE"

EXIT=0
for sha in `git log --format=oneline "$RANGE" | cut '-d ' -f1`
do
    echo -e "\nChecking commit message for $sha ..."
    VALUE=0

    line_no=0
	while read -r line; do
		printf "%02d (%03d) : %s \n" "$line_no" "${#line}" "$line"

		if [[ $line == "Merge pull request"* ]]; then
			echo -e "Merge pull request, SKIP"
			break
	    elif [[ ${#line} -gt 72 ]] && [[ line_no -ne 1 ]]; then
	    	echo "Line to long, should not exceed 72 characters"
	    	exit 2
	    elif [[ ${#line} -gt 0 ]] && [[ line_no -eq 1 ]]; then
	    	echo "2nd line should be empty"
	    	exit 3
	    fi

	    line_no=$((line_no+1))
	done <<< "$(git log --format=%B -n 1 $sha)"
done
