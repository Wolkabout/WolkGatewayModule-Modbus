#!/bin/bash

if [ "$EUID" -ne 0 ]
  then echo "Please run as sudo."
  exit
fi

./build-image-armv7l.sh

docker container stop debuilder
docker container rm debuilder

docker run -dit --name debuilder --cpus $(nproc) wolkabout:wgmm-armv7l || exit
docker exec -it debuilder /build/make_deb.sh || exit
docker cp debuilder:/build/ .

docker container stop debuilder
docker container rm debuilder

mv ./build/*.deb .
rm -rf ./build/

chown "$USER:$USER" *.deb
