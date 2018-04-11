/*
 * Copyright 2018 WolkAbout Technology s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MODBUSCLIENT_H
#define MODBUSCLIENT_H

#include <string>

namespace wolkabout
{
class ModbusClient
{
public:
    virtual ~ModbusClient() = default;

    virtual bool connect() = 0;
    virtual bool disconnect() = 0;

    virtual bool isConnected() = 0;

    virtual bool writeHoldingRegister(int address, signed short value) = 0;
    virtual bool writeHoldingRegister(int address, unsigned short value) = 0;
    virtual bool writeHoldingRegister(int address, float value) = 0;

    virtual bool writeCoil(int address, bool value) = 0;

    virtual bool readInputRegister(int address, signed short& value) = 0;
    virtual bool readInputRegister(int address, unsigned short& value) = 0;
    virtual bool readInputRegister(int address, float& value) = 0;

    virtual bool readInputBit(int address, bool& value) = 0;

    virtual bool readHoldingRegister(int address, signed short& value) = 0;
    virtual bool readHoldingRegister(int address, unsigned short& value) = 0;
    virtual bool readHoldingRegister(int address, float& value) = 0;

    virtual bool readCoil(int address, bool& value) = 0;
};
}    // namespace wolkabout

#endif
