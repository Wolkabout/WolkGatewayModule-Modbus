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
                             ModbusConfiguration::BitParity bitParity, std::chrono::milliseconds responseTimeout);

    virtual ~LibModbusSerialRtuClient();

private:
    bool createContext() override;
    bool destroyContext() override;

    bool writeHoldingRegister(int address, signed short value) override;
    bool writeHoldingRegister(int address, unsigned short value) override;
    bool writeHoldingRegister(int address, float value) override;

    bool writeCoil(int address, bool value) override;

    bool readInputRegister(int address, signed short& value) override;
    bool readInputRegisters(int address, int number, std::vector<signed short>& values) override;
    bool readInputRegister(int address, unsigned short& value) override;
    bool readInputRegisters(int address, int number, std::vector<unsigned short>& values) override;
    bool readInputRegister(int address, float& value) override;
    bool readInputRegisters(int address, int number, std::vector<float>& values) override;

    bool readInputContact(int address, bool& value) override;
    bool readInputContacts(int address, int number, std::vector<bool>& values) override;

    bool readHoldingRegister(int address, signed short& value) override;
    bool readHoldingRegisters(int address, int number, std::vector<signed short>& values) override;
    bool readHoldingRegister(int address, unsigned short& value) override;
    bool readHoldingRegisters(int address, int number, std::vector<unsigned short>& values) override;
    bool readHoldingRegister(int address, float& value) override;
    bool readHoldingRegisters(int address, int number, std::vector<float>& values) override;

    bool readCoil(int address, bool& value) override;
    bool readCoils(int address, int number, std::vector<bool>& values) override;

    void sleepBetweenModbusMessages() const;

    std::string m_serialPort;
    int m_baudRate;
    char m_dataBits;
    char m_stopBits;
    char m_bitParity;
};
}    // namespace wolkabout

#endif
