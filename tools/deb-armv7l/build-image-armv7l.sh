#!/bin/bash

if [ "$EUID" -ne 0 ]
  then echo "Please run as sudo."
  exit
fi

cp ../make_deb.sh .

docker build -t wolkabout:wgmm-armv7l .
rm make_deb.sh