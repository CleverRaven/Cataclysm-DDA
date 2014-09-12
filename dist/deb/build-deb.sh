#!/bin/bash

# which-based requirements check.
function check_requirements {
    err_flag=false
    for prog_name in $@
    do
        if ! which $prog_name 1>/dev/null
        then
            err_flag=true
            echo "Not found: $prog_name"
        fi
    done
    if [ "$err_flag" = true ]
    then
        exit 1
    fi
}

## Script should be running from cdda root.
# Simple check existense of "./src/main.cpp"
if ! [ -f "./src/main.cpp" ]
then
    echo "You need to run this script ONLY from CDDA root."
    exit 1
fi

echo "Checking requirements..."
REQ="git dpkg-buildpackage gpg grep"
check_requirements "$REQ"

## Get maintainer name and e-mail
read -p "Set maintainer (FirstName LastName <e-mail>):" MAINT
MAINT_EMAIL=$(echo "$MAINT" | grep -Po '\<(\K[^\>]+)' | grep '@')
if [ -z "$MAINT_EMAIL" ]
then
    echo "Wrong maintainer e-mail."
    exit 1
fi

## Check gpg key for maintaier e-mail.
# TODO Signing is not used now.
#DPKG_SIGN="--force-sign"
#DPKG_ARGS="-b $DPKG_SIGN"
#echo "Cheking gpg key of maintainer..."
#if ! gpg --list-keys "$MAINT_EMAIL" &> /dev/null
#then
#    echo "Public key of maintainer was not found."
#    PS3="What next:"
#    CH1="Generate new GPG key for signing."
#    CH2="Continue without package signing."
#    CH3="Quit."
#    select choose in "$CH1" "$CH2" "$CH3"
#    do
#        case "$REPLY" in
#            1 ) echo "Let's $opt"; break;;
#            2 ) DPKG_SIGN="-us -uc"; break;;
#            3 ) exit 0;;
#        esac
#    done
#    exit 1
    ## TODO Generate key (y/n). Disable signing flag
#fi

## Copy debian/ into root dir.
echo "Сopying debian dir into cdda root..."
rm -rf "./debian/*"
cp -rf "dist/deb/debian" "."

## Setup Maintainer info.
sed "s/<NO_MAINTAINER>/$MAINT/" dist/deb/debian/control > ./debian/control

## Building packages
PACKAGES="cataclysm-dda cataclysm-dda-tiles cataclysm-dda-common cataclysm-dda-gfx"
echo "Building packages..."
for p in $PACKAGES
do
    dpkg-buildpackage -T "$p"
done
