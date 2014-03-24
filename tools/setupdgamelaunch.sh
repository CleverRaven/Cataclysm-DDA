#!/bin/bash

if (pwd | grep "Cataclysm-DDA/tools")
then
cd ..
else
git clone https://github.com/CleverRaven/Cataclysm-DDA
cd Cataclysm-DDA
fi

make

cd ..

git clone https://github.com/C0DEHERO/dgamelaunch
cd dgamelaunch

./autogen.sh --enable-sqlite --enable-shmem --with-config-file=/opt/dgamelaunch/cdda/etc/dgamelaunch.conf
make
sudo ./dgl-create-chroot
