#!/bin/bash

apt install -y debhelper devscripts

mkdir -p ./tmp-wgmm-deb
cd ./tmp-wgmm-deb || exit
git clone https://github.com/Wolkabout/WolkGatewayModule-Modbus --recurse-submodules
cd ./WolkGatewayModule-Modbus || exit

# TODO Remove when merge to master
git checkout debian_package

debuild -us -uc -b -j$(nproc)
cd ../
mv *.deb ..
cd ../
rm -rf ./tmp-wgmm-deb

