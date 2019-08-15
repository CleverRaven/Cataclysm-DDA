#!/bin/bash

# This is a helper script intended to assist with running clang-tidy repeatedly
# across the codebase.
# It runs clang-tidy on all files (or, if you provide suitable arguments, all
# files matching a particular regex).
# It notes any file which had an error, and runs again on all those files.
# It repeats until there are no more errors.
# This is useful to catch refactoring changes from checks which open the
# possibility for more refactoring changes.
# There are a couple of limitations:
# - If an error has no FIX-IT, then the script will just run forever repeating
#   that error.  You can go fix it by hand while the script is still running.
# - If you run clang-tidy in parallel (pass the number of jobs as an argument
#   to this script) and multiple clang-tidy runs try to fix the same header at
#   the same time, it will end up getting messed up.  clang-tidy does support
#   doing such parallel fixes properly, but I haven't bothered to figure out
#   how.

set -eu

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
top_dir="$(dirname "$script_dir")"

if [ $# -ge 1 ]
then
    jobs="$1"
else
    jobs=1
fi

if [ $# -ge 2 ]
then
    file_regex="$1"
else
    file_regex='.'
fi

list_of_files=$(grep '"file": "' build/compile_commands.json | \
    sed "s+.*$PWD/++;s+\"$++" | \
    egrep "$file_regex")

plugin_lib="$top_dir/build/tools/clang-tidy-plugin/libCataAnalyzerPlugin.so"
plugin_opt=
if [ -r "$plugin_lib" ]
then
    plugin_opt="-plugins=$plugin_lib"
fi

temp_file=$(mktemp)
trap "rm -f $temp_file" EXIT

while [ -n "$list_of_files" ]
do
    exec 3>$temp_file
    printf "Running clang-tidy on %d files\n" \
        "$(($(printf "%s" "$list_of_files" | wc -l)+1))"
    printf "%s" "$list_of_files" | \
        nice -15 xargs -P "$jobs" -n 1 \
        "$script_dir/repeat_clang_tidy_helper.sh" \
        -quiet -fix ${plugin_opt:+"$plugin_opt"}
    list_of_files="$(cat $temp_file)"
done
