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

#ifndef LIBMODBUSSERIALRTUCLIENT_H
#define LIBMODBUSSERIALRTUCLIENT_H

#include "modbus/ModbusClient.h"
#include "modbus/ModbusConfiguration.h"
#include "modbus/libmodbus/modbus.h"

#include <chrono>
#include <mutex>
#include <string>

namespace wolkabout
{
class LibModbusSerialRtuClient : public ModbusClient
{
public:
    LibModbusSerialRtuClient(std::string serialPort, int baudRate, char dataBits, char stopBits,
                             ModbusConfiguration::BitParity bitParity, int slaveAddress,
                             std::chrono::milliseconds responseTimeout);

    virtual ~LibModbusSerialRtuClient();

    bool connect() override;
    bool disconnect() override;

    bool isConnected() override;

    bool writeHoldingRegister(int address, signed short value) override;
    bool writeHoldingRegister(int address, unsigned short value) override;
    bool writeHoldingRegister(int address, float value) override;

    bool writeCoil(int address, bool value) override;

    bool readInputRegister(int address, signed short& value) override;
    bool readInputRegister(int address, unsigned short& value) override;
    bool readInputRegister(int address, float& value) override;

    bool readInputBit(int address, bool& value) override;

    bool readHoldingRegister(int address, signed short& value) override;
    bool readHoldingRegister(int address, unsigned short& value) override;
    bool readHoldingRegister(int address, float& value) override;

    bool readCoil(int address, bool& value) override;

private:
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

    std::string m_serialPort;
    int m_baudRate;
    char m_dataBits;
    char m_stopBits;
    char m_bitParity;
    int m_slaveAddress;

    std::chrono::milliseconds m_responseTimeout;

    std::mutex m_modbusMutex;
    modbus_t* m_modbus;
};
}    // namespace wolkabout

#endif
