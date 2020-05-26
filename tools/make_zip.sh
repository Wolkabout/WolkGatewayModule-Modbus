#!/bin/bash

# This script is used to make a release .zip file for WolkGatewayModule-Modbus release

mkdir -p ./tmp-wgmm
cd ./tmp-wgmm
git clone https://github.com/Wolkabout/WolkGatewayModule-Modbus --recurse-submodules
cd ./WolkGatewayModule-Modbus

# TODO remove line
git checkout debian_package

filename="WolkGateway-ModbusModule-v$(cat RELEASE_NOTES.txt | grep "**Version" | head -1 | sed -e "s/**Version //" | sed -e "s/\*\*//").zip"
echo "filename: $filename"
zip -qr $filename *
mv "$filename" ../..
cd ../..
rm -rf ./tmp-wgmm
