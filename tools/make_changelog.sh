#!/bin/bash

GITURL='https://api.github.com/repos/CleverRaven/Cataclysm-DDA'
CHANGELOG=../changelog
ADDCHANGE=./add_change.sh
PARSESUM=./parse_summary.py
num_new=0

function usage() {
    echo "Usage: $0 [options] <git range> "
    echo -e "\t-u, --user      git user credentials/token"
    echo -e "\t-m, --max       maximum git api requests to make"
    echo ""
    echo -e "\tE.g.: $0 -u 'user:pw' 0ab93ad..HEAD"
}

while [[ "$1" != "" ]]; do
    case $1 in
        -h|--help)
            usage
            exit 0
            ;;
        -u|--user)
            GITUSER=$2
            shift;
            ;;
        -m|--max)
            MAXREQ=$2
            shift;
            ;;
        *)
            RANGE=$1
            shift;
            ;;
    esac
    shift
done

if [[ ! -f $ADDCHANGE ]] || [[ ! -f $PARSESUM ]]; then
    echo "Failed to find helper scripts $ADDCHANGE/$PARSESUM"
    exit 2
fi

if [[ $MAXREQ -gt 5000 ]]; then
    echo "Max requests cannot be greater than 5000"
    usage
    exit 1
elif [[ "$MAXREQ" == "" ]]; then
    if [[ "$GITUSER" == "" ]]; then
        MAXREQ=60
    else
        MAXREQ=5000
    fi
fi

if [[ "$RANGE" == "" ]]; then
    echo "No range specified"
    usage
    exit 0
fi

# Find all the merge requests in the git log
pullreqs=($(git log --pretty=oneline $RANGE | cut -d' ' -f 2- | \
    sed -nr  's/^Merge pull request #([0-9]*) from .*$/\1/p'))

# Cache the existing changes in the changelog to avoid needless curl requests
declare -A old_changes
while read -r line; do
    if [[ "$line" != "" ]]; then
        split=($(echo $line | tr ':' '\n'))
        # If there's changes in the change log without a PR ID we don't need to
        # cache them
        if [[ ${#split[@]} -gt 1 ]]; then
            old_changes["${split[0]}"]="${split[@]:1}"
        fi
    fi
done <<<$(cat $CHANGELOG | sed -n '/\W*[0-9]*: /p')

# For each pull request found in range, check if they already exist in the
# changelog. If not, add them.
for pr in "${pullreqs[@]}"; do
    if [[ $num_new -ge $MAXREQ ]]; then
        echo "Hit maximum number of requests."
        break
    fi

    if [[ "${old_changes[$pr]}" == "" ]]; then
        echo "Found new PR: $pr"
        if [[ "$GITUSER" != "" ]]; then
            newpr=$(curl -s -u $GITUSER "$GITURL/pulls/$pr" | $PARSESUM)
        else
            newpr=$(curl -s "$GITURL/pulls/$pr" | $PARSESUM)
        fi
        # $PARSESUM returns non zero then something went wrong
        if [[ $? -eq 0 ]]; then
            echo "Adding: $newpr"
            $ADDCHANGE $newpr
            num_new=$((num_new + 1))
        else
            echo "An error occurred trying to retrieve PR info from $GITURL"
            echo "Aborting."
            exit 1
        fi
    fi
done
echo "Added $num_new new changes to changelog"
