#!/bin/bash
# adapted from https://marcelbraghetto.github.io/a-simple-triangle/

# Given the name of a Homebrew formula, check if its installed and if not, install it.
fetch_brew_dependency() {
    FORMULA_NAME=$1

    echo "Checking for Brew dependency: '$FORMULA_NAME'."

    if brew ls --versions $FORMULA_NAME > /dev/null; then
        echo "Dependency '$FORMULA_NAME' is already installed, continuing ..."
    else
        echo "Dependency '$FORMULA_NAME' is not installed, installing via Homebrew ..."
        brew install $FORMULA_NAME
    fi
}

fetch_brew_dependency "xcodegen"

echo "Checking for build folder ..."
if [ ! -d "../build" ]; then
    mkdir ../build
fi

echo "Checking for libs folder ..."
if [ ! -d "../build/libs" ]; then
    echo "ERROR: Static SDL libraries not found at /build/libs ..."
    echo "Reffer to /doc/COMPILING/COMPILING-XCODE.md for instructions on how to build the static libraries."
    exit
fi


if [ ! -f "xcode_dev_id.yml" ]; then
    echo "xcode_dev_id.yml not found -"
    echo "  You will need to manually specify your Development Team for signing in Xcode."
    echo "  See the following link for details:"
    echo "  https://help.apple.com/xcode/mac/current/#/dev23aab79b4"
    echo ""
    echo "To avoid this message in the future see the xcode_dev_id_example.yml file."
fi
# Invoke the xcodegen tool to create our project file.
echo "Generating Xcode project"
xcodegen -s xcodegen-cataclysm.yml -p ../build

if [ ! -f "../build/CataclysmExperimental.xcodeproj" ]; then
    echo "Project built successfully. Launching Xcode..."
    open ../build/CataclysmExperimental.xcodeproj
    exit
else
    echo "Error in project creation process. Review output above for errors."
fi