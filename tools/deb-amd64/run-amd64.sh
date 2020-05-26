#!/bin/bash

if [ "$EUID" -ne 0 ]
  then echo "Please run as sudo."
  exit
fi

./build-image-amd64.sh

docker container stop debuilder
docker container rm debuilder

docker run -dit --name debuilder --cpus $(nproc) wolkabout:wgmm-amd64 || exit
docker exec -it debuilder /build/make_deb.sh || exit
docker cp debuilder:/build/ .

docker container stop debuilder
docker container rm debuilder

mv ./build/*.deb .
rm -rf ./build/

chown "$USER:$USER" *.deb
