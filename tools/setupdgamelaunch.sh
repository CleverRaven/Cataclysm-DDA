#!/bin/sh

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

if (ls dgamelaunch)
then
echo "dgamelaunch already exists"
else
git clone https://github.com/C0DEHERO/dgamelaunch
fi
cd dgamelaunch

./autogen.sh --enable-sqlite --enable-shmem --with-config-file=/opt/dgamelaunch/cdda/etc/dgamelaunch.conf
make
sudo ./dgl-create-chroot
