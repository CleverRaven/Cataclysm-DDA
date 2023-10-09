#!/bin/bash

# This is a helper script intended to assist with running clang-tidy on Amazon
# Lambda via llama.  This can dramatically accelerate clang-tidy runs,
# something which can otherwise be quite slow!

set -eu

if [ $# -ge 1 ]
then
    file_regex="$1"
else
    file_regex='.'
fi

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
top_dir="$(dirname "$script_dir")"

list_of_files=$(grep '"file": "' build/compile_commands.json | \
    sed "s+.*$PWD/++;s+\"$++" | \
    egrep "$file_regex")

plugin_lib="$top_dir/build/tools/clang-tidy-plugin/libCataAnalyzerPlugin.so"
plugin_opt=
if [ -r "$plugin_lib" ]
then
    plugin_opt="-plugins=$plugin_lib"
fi

printf '%s\n' "$list_of_files" | \
    llama xargs -logs -j 10 gcc \
        clang-tidy -quiet ${plugin_opt:+"$plugin_opt"} \
        -export-fixes='{{.O (printf "fixes/%s" .Line)}}' '{{.I .Line}}'
