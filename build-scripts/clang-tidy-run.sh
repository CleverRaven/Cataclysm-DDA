#!/bin/bash

# Shell script intended for clang-tidy check

echo "Using bash version $BASH_VERSION"
set -exo pipefail

num_jobs=3

# enable all the switches by default
BACKTRACE=${BACKTRACE:-1}
LOCALIZE=${LOCALIZE:-1}
TILES=${TILES:-1}
SOUND=${SOUND:-1}

# create compilation database (compile_commands.json)
mkdir -p build
cd build
cmake \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    ${COMPILER:+-DCMAKE_CXX_COMPILER=$COMPILER} \
    -DCMAKE_BUILD_TYPE="Release" \
    -DBACKTRACE=${BACKTRACE} \
    -DLOCALIZE=${LOCALIZE} \
    -DTILES=${TILES} \
    -DSOUND=${SOUND} \
    ..
cd ..
ln --force --symbolic build/compile_commands.json .

if [ ! -f build/tools/clang-tidy-plugin/libCataAnalyzerPlugin.so ]
then
    echo "Cata plugin not found. Assuming we're in CI and bailing out."
    echo "If you are running clang-tidy locally with no plugin, consider"
    echo "calling it explicitly with the files you care to check."
    echo 'e.g. `clang-tidy src/item* tests/item*` '
    exit 1
fi

# Show compiler C++ header search path
${COMPILER:-clang++} -v -x c++ /dev/null -c
# And the same for clang-tidy
./build-scripts/clang-tidy-wrapper.sh --version
# list of checks
./build-scripts/clang-tidy-wrapper.sh --list-checks

# We want to first analyze all files that changed in this PR, then as
# many others as possible, in a random order.
set +x

# Check for changes to any files that would require us to run clang-tidy across everything
changed_global_files="$( ( cat ./files_changed || echo 'unknown' ) | \
    egrep -i "clang-tidy-build.sh|clang-tidy-run.sh|clang-tidy-wrapper.sh|clang-tidy.yml|.clang-tidy|files_changed|get_affected_files.py|CMakeLists.txt|CMakePresets.json|unknown" || true )"
if [ -n "$changed_global_files" ]
then
    first_changed_file="$(echo "$changed_global_files" | head -n 1)"
    echo "Analyzing all files because $first_changed_file was changed"
    TIDY="all"
fi

all_cpp_files="$(jq -r '.[].file | select(contains("third-party") | not)' build/compile_commands.json)"
if [ "$TIDY" == "all" ]
then
    echo "Analyzing all files"
    tidyable_cpp_files=$all_cpp_files
else
    make \
        --silent \
        -j $num_jobs \
        ${COMPILER:+COMPILER=$COMPILER} \
        BACKTRACE=${BACKTRACE} \
        LOCALIZE=${LOCALIZE} \
        TILES=${TILES} \
        SOUND=${SOUND} \
        includes

    tidyable_cpp_files="$( \
        ( test -f ./files_changed && ( build-scripts/get_affected_files.py --changed-files-list ./files_changed ) ) || \
        echo unknown )"

    tidyable_cpp_files="$(echo -n "$tidyable_cpp_files" | grep -v third-party || true)"
    if [ -z "$tidyable_cpp_files" ]
    then
	echo "No files to tidy, exiting";
	set -x
	exit 0
    fi
    if [ "$tidyable_cpp_files" == "unknown" ]
    then
        echo "Unable to determine affected files, tidying all files"
        tidyable_cpp_files=$all_cpp_files
    fi
fi

printf "Subset to analyze: '%s'\n" "$CATA_CLANG_TIDY_SUBSET"

# (temporary create ./files_changed and then clean up it later. This is a terrible hack, and I'm not proud)
if [ ! -f ./files_changed ] ; then touch ./files_changed ; CLEANUP_FILES_CHANGED="yes" ; fi

# We might need to analyze only a subset of the files if they have been split
# into multiple jobs for efficiency. The paths from `compile_commands.json` can
# be absolute but the paths from `get_affected_files.py` are relative, so both
# formats are matched. Exit code 1 from grep (meaning no match) is ignored in
# case one subset contains no file to analyze.
case "$CATA_CLANG_TIDY_SUBSET" in
    ( directly-changed )
        tidyable_cpp_files=$(printf '%s\n' "$tidyable_cpp_files" | grep -f ./files_changed || [[ $? == 1 ]])
        ;;
    ( indirectly-changed-src )
        tidyable_cpp_files=$(printf '%s\n' "$tidyable_cpp_files" | grep -E '(^|/)src/' | grep -vf ./files_changed || [[ $? == 1 ]])
        ;;
    ( indirectly-changed-other )
        tidyable_cpp_files=$(printf '%s\n' "$tidyable_cpp_files" | grep -Ev '(^|/)src/' | grep -vf ./files_changed || [[ $? == 1 ]])
        ;;
esac
if [ "${CLEANUP_FILES_CHANGED}" == "yes" ] ; then rm -f ./files_changed ; fi

printf "full list of files to analyze (they might get shuffled around in practice):\n%s\n" "$tidyable_cpp_files"

function analyze_files_in_random_order
{
    if [ -n "$1" ]
    then
        echo "$1" | shuf | \
            xargs -P "$num_jobs" -n 1 ./build-scripts/clang-tidy-wrapper.sh -quiet
    else
        echo "No files to analyze"
    fi
}

echo "Analyzing affected files"
analyze_files_in_random_order "$tidyable_cpp_files"
set -x
