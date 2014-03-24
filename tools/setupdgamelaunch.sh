#!/bin/bash
#git clone https://github.com/CleverRaven/Cataclysm-DDA // you should already have that
#cd Cataclysm-DDA
cd ..
make

cd ..

git clone https://github.com/C0DEHERO/dgamelaunch
cd dgamelaunch

./autogen.sh --enable-sqlite --enable-shmem --with-config-file=/opt/dgamelaunch/cdda/etc/dgamelaunch.conf
make
./dgl-create-chroot
