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

# libmodbus
if [ ! -d "libmodbus" ]; then
	echo "Downloading libmodbus"
	wget -O libmodbus.tar.gz https://github.com/stephane/libmodbus/archive/v3.1.4.tar.gz
	tar -xvzf libmodbus.tar.gz
	mv libmodbus-3.1.4 libmodbus
	rm libmodbus.tar.gz
fi
