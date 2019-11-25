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

#include "modbus/libmodbus/modbus.h"

#include <chrono>
#include <mutex>
#include <string>
#include <vector>

namespace wolkabout
{
class ModbusClient
{
public:
    ModbusClient(std::chrono::milliseconds responseTimeout);
    virtual ~ModbusClient() = default;

    bool connect();
    bool disconnect();

    bool isConnected();

    bool writeHoldingRegister(int slaveAddress, int address, signed short value);
    bool writeHoldingRegister(int slaveAddress, int address, unsigned short value);
    bool writeHoldingRegister(int slaveAddress, int address, float value);

    bool writeCoil(int slaveAddress, int address, bool value);

    bool readInputRegisters(int slaveAddress, int address, int number, std::vector<signed short>& values);
    bool readInputRegisters(int slaveAddress, int address, int number, std::vector<unsigned short>& values);
    bool readInputRegisters(int slaveAddress, int address, int number, std::vector<float>& values);

    bool readInputContacts(int slaveAddress, int address, int number, std::vector<bool>& values);

    bool readHoldingRegister(int slaveAddress, int address, signed short& value);
    bool readHoldingRegisters(int slaveAddress, int address, int number, std::vector<signed short>& values);
    bool readHoldingRegister(int slaveAddress, int address, unsigned short& value);
    bool readHoldingRegisters(int slaveAddress, int address, int number, std::vector<unsigned short>& values);
    bool readHoldingRegister(int slaveAddress, int address, float& value);
    bool readHoldingRegisters(int slaveAddress, int address, int number, std::vector<float>& values);

    bool readCoil(int slaveAddress, int address, bool& value);
    bool readCoils(int slaveAddress, int address, int number, std::vector<bool>& values);

protected:
    virtual bool createContext() = 0;
    virtual bool destroyContext() = 0;

    virtual bool writeHoldingRegister(int address, signed short value);
    virtual bool writeHoldingRegister(int address, unsigned short value);
    virtual bool writeHoldingRegister(int address, float value);

    virtual bool writeCoil(int address, bool value);

    virtual bool readInputRegisters(int address, int number, std::vector<signed short>& values);
    virtual bool readInputRegisters(int address, int number, std::vector<unsigned short>& values);
    virtual bool readInputRegisters(int address, int number, std::vector<float>& values);

    virtual bool readInputContacts(int address, int number, std::vector<bool>& values);

    virtual bool readHoldingRegister(int address, signed short& value);
    virtual bool readHoldingRegisters(int address, int number, std::vector<signed short>& values);
    virtual bool readHoldingRegister(int address, unsigned short& value);
    virtual bool readHoldingRegisters(int address, int number, std::vector<unsigned short>& values);
    virtual bool readHoldingRegister(int address, float& value);
    virtual bool readHoldingRegisters(int address, int number, std::vector<float>& values);

    virtual bool readCoil(int address, bool& value);
    virtual bool readCoils(int address, int number, std::vector<bool>& values);

    virtual bool changeSlaveAddress(int address);

    union ModbusValue {
        unsigned short unsignedShortValues[2];
        signed short signedShortValue;
        unsigned short unsignedShortValue;
        float floatValue;

        ModbusValue()
        {
            unsignedShortValues[0] = 0u;
            unsignedShortValues[1] = 0u;

            signedShortValue = 0;
            unsignedShortValue = 0u;
            floatValue = 0.0f;
        }
    };

    std::chrono::milliseconds m_responseTimeout;
    const int m_timeoutDurations[10] = {0, 5, 10, 15, 30, 60, 300, 600, 1800, 3600};
    int m_timeoutIterator;

    bool m_connected;
    bool m_contextCreated;
    std::mutex m_modbusMutex;
    modbus_t* m_modbus;
};
}    // namespace wolkabout

#endif
