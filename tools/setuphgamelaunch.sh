#!/bin/bash

if (pwd | grep "Cataclysm-DDA/tools")
then
cd ..
else
if (ls Cataclysm-DDA)
then
echo "Cataclysm-DDA already exists"
else
git clone https://github.com/CleverRaven/Cataclysm-DDA
fi
cd Cataclysm-DDA
fi

make

cd ..

if (ls Hgamelaunch)
then
echo "Hgamelaunch already exists"
else
git clone https://github.com/C0DEHERO/Hgamelaunch
fi
cd Hgamelaunch

cabal configure
cabal build

# Setup folder structure
#ROOTDIR = $1
mkdir -p $1
echo "Copying bin to $1"
cp ./dist/build/Hgamelaunch/Hgamelaunch $1
echo "Copying config dir to $1/config"
cp -r ./config $1
echo "Copying license to $1"
cp ./LICENSE.md $1
echo "Copying readme to $1"
cp ./README.md $1
