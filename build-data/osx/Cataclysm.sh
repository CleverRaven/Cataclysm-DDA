#!/bin/bash
# Set a minimal PATH
export PATH="/usr/bin:/bin:/usr/sbin:/sbin"

# Determine the directory of this script and change to the Resources directory
SCRIPT_DIR="$(dirname "$0")"
RESOURCES_DIR="${SCRIPT_DIR}/../Resources"
cd "$RESOURCES_DIR" || { echo "Error: Cannot change directory to ${RESOURCES_DIR}"; exit 1; }

# Get the kernel release major version
KERNEL_RELEASE=$(uname -r | cut -d. -f1)
if [ "$KERNEL_RELEASE" -ge 11 ]; then
    LIBRARY_VAR="DYLD_LIBRARY_PATH"
    FRAMEWORK_VAR="DYLD_FRAMEWORK_PATH"
else
    LIBRARY_VAR="DYLD_FALLBACK_LIBRARY_PATH"
    FRAMEWORK_VAR="DYLD_FALLBACK_FRAMEWORK_PATH"
fi

# If the 'cataclysm' executable exists, launch it in a new Terminal window;
# otherwise, if 'cataclysm-tiles' exists, run it in the current shell.
if [ -f "./cataclysm" ]; then
    # Build a shell command to export environment variables,
    # change to the current directory, run the executable, and then exit.
    SHELL_CMD="export $LIBRARY_VAR=. && export $FRAMEWORK_VAR=. && cd \"$(pwd)\" && ./cataclysm; exit"
    osascript -e "tell application \"Terminal\" to do script \"$SHELL_CMD\""
    osascript -e "tell application \"Terminal\" to activate"
elif [ -f "./cataclysm-tiles" ]; then
    # Export the required environment variables and run the tiles version
    export ${LIBRARY_VAR}="."
    export ${FRAMEWORK_VAR}="."
    ./cataclysm-tiles
else
    echo "Error: Neither 'cataclysm' nor 'cataclysm-tiles' exists in ${RESOURCES_DIR}."
    exit 1
fi
