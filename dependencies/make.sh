#!/usr/bin/env bash

# Copyright 2018 WolkAbout Technology s.r.o.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

BASE_DIR="$(pwd)"

BUILD_DIR="$BASE_DIR/build"
LIB_DIR="$BUILD_DIR/lib"

LIB_DEPLOY_DIR="$BASE_DIR/../out/lib"
INCLUDE_DEPLOY_DIR="$BASE_DIR/../src"

LIB_EXTENSION="so"

if [[ `uname` == "Darwin" ]]; then
    LIB_EXTENSION="dylib"
fi

mkdir -pv "$BUILD_DIR"


# libmodbus
if [ ! -f "$LIB_DIR/libmodbus.$LIB_EXTENSION" ]; then
    echo "Building libmodbus"
    pushd libmodbus

    sh autogen.sh
    ./configure --prefix=$BASE_DIR --includedir=$BASE_DIR/carwash/include  \
                --libdir=$LIB_DIR --disable-tests

    make -j8 && make install
    popd
fi

# Copy shared libraries
mkdir -p $LIB_DEPLOY_DIR


# libmodbus
cp $BASE_DIR/libmodbus/src/*.h $INCLUDE_DEPLOY_DIR/modbus/libmodbus
cp $BUILD_DIR/lib/libmodbus.*$LIB_EXTENSION* $LIB_DEPLOY_DIR
