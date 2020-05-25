#!/bin/bash

if [ $( dpkg -l debhelper | wc -l ) -le 6 ]
then
  apt install debhelper -y || exit
fi

if [ $( dpkg -l devscripts | wc -l ) -le 6 ]
then
  apt install devscripts -y || exit
fi

mkdir -p ./tmp-wgmm-deb
cd ./tmp-wgmm-deb || exit
git clone https://github.com/Wolkabout/WolkGatewayModule-Modbus --recurse-submodules
cd ./WolkGatewayModule-Modbus || exit
# TODO Remove when merge to master
git checkout debian_package
debuild -us -uc -b
cd ../
mv *.deb ..
cd ../
rm -rf ./tmp-wgmm-deb

