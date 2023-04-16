#!/bin/bash

set -eu

filename="${!#}"
printf "%s\n" "$filename"
tmpdir=$(mktemp --tmpdir -d repeat_clang_tidy.XXXXXX)
tmpfile="$tmpdir"/fixes.yaml
trap 'rm -rf "$tmpdir"' EXIT
start_time=$(date +%s.%N)
clang-tidy --export-fixes="$tmpfile" "$@" || printf "%s\n" "$filename" >&3

if [ ! -r "$tmpfile" ]
then
    exit 0
fi

files_modified=$( \
    grep -A1 Replacements: "$tmpfile" | \
    grep 'FilePath:' | \
    awk '{print $3}' | \
    tr -d "'" \
)

if [ -z "$files_modified" ]
then
    echo 'Fixes file created but no filenames found'
    exit 0
fi

# If none of the files have changed since we started running, then apply the
# fixes.  We need to do this under a lock to ensure that at most one script
# updates at a time
lockdir=repeat_clang_tidy.lock
while ! mkdir "$lockdir"
do
    echo 'Waiting for lock...'
    sleep 0.1
done

trap 'rm -rf "$lockdir" "$tmpdir"' EXIT

for file in $files_modified
do
    mtime=$(find "$file" -printf %T@)
    if awk -v n1=$start_time -v n2=$mtime 'BEGIN{exit n1 > n2}'
    then
        printf '%s was modified since clang-tidy run; aborting changes\n' "$file"
        exit 0
    fi
done

clang-apply-replacements $tmpdir
