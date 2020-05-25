#!/bin/bash

mkdir -p ./tmp-wgmm-deb
cd ./tmp-wgmm-deb
git clone https://github.com/Wolkabout/WolkGatewayModule-Modbus --recurse-submodules
cd ./WolkGatewayModule-Modbus
git checkout debian_package
debuild -us -uc -b
cd ../../
rm -rf ./tmp-wgmm-deb

