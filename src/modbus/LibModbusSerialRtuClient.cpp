/*
 * Copyright 2020 WolkAbout Technology s.r.o.
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

#include "modbus/LibModbusSerialRtuClient.h"
#include "modbus/ModbusClient.h"
#include "modbus/libmodbus/modbus.h"
#include "utilities/Logger.h"

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <utility>

namespace wolkabout
{
LibModbusSerialRtuClient::LibModbusSerialRtuClient(std::string serialPort, int baudRate, char dataBits, char stopBits,
                                                   SerialRtuConfiguration::BitParity bitParity,
                                                   std::chrono::milliseconds responseTimeout)
: ModbusClient(std::move(responseTimeout))
, m_serialPort(std::move(serialPort))
, m_baudRate(baudRate)
, m_dataBits(dataBits)
, m_stopBits(stopBits)
{
    switch (bitParity)
    {
    case SerialRtuConfiguration::BitParity::NONE:
        m_bitParity = 'N';
        break;

    case SerialRtuConfiguration::BitParity::EVEN:
        m_bitParity = 'E';
        break;

    case SerialRtuConfiguration::BitParity::ODD:
        m_bitParity = 'O';
        break;
    }
}

LibModbusSerialRtuClient::~LibModbusSerialRtuClient()
{
    destroyContext();
}

bool LibModbusSerialRtuClient::createContext()
{
    LOG(INFO) << "LibModbusClient: Opening serial port '" << m_serialPort << "'  Baud: " << m_baudRate;

    m_modbus = modbus_new_rtu(m_serialPort.c_str(), m_baudRate, m_bitParity, m_dataBits, m_stopBits);
    if (m_modbus == nullptr)
    {
        LOG(ERROR) << "LibModbusClient: Unable to create modbus context - " << modbus_strerror(errno);
        return false;
    }

    m_contextCreated = true;
    return true;
}

bool LibModbusSerialRtuClient::destroyContext()
{
    if (m_modbus)
    {
        LOG(INFO) << "LibModbusClient: Closing serial port '" << m_serialPort << "'";
        disconnect();
        modbus_free(m_modbus);
        m_modbus = nullptr;
        m_contextCreated = false;
    }

    return true;
}

bool LibModbusSerialRtuClient::writeHoldingRegister(int address, signed short value)
{
    auto result = ModbusClient::writeHoldingRegister(address, value);

    sleepBetweenModbusMessages();
    return result;
}

bool LibModbusSerialRtuClient::writeHoldingRegister(int address, unsigned short value)
{
    auto result = ModbusClient::writeHoldingRegister(address, value);

    sleepBetweenModbusMessages();
    return result;
}

bool LibModbusSerialRtuClient::writeHoldingRegister(int address, float value)
{
    auto result = ModbusClient::writeHoldingRegister(address, value);

    sleepBetweenModbusMessages();
    return result;
}

bool LibModbusSerialRtuClient::writeCoil(int address, bool value)
{
    auto result = ModbusClient::writeCoil(address, value);

    sleepBetweenModbusMessages();
    return result;
}

bool LibModbusSerialRtuClient::readInputRegisters(int address, int number, std::vector<signed short>& values)
{
    auto result = ModbusClient::readInputRegisters(address, number, values);

    sleepBetweenModbusMessages();
    return result;
}

bool LibModbusSerialRtuClient::readInputRegisters(int address, int number, std::vector<unsigned short>& values)
{
    auto result = ModbusClient::readInputRegisters(address, number, values);

    sleepBetweenModbusMessages();
    return result;
}

bool LibModbusSerialRtuClient::readInputRegisters(int address, int number, std::vector<float>& values)
{
    auto result = ModbusClient::readInputRegisters(address, number, values);

    sleepBetweenModbusMessages();
    return result;
}

bool LibModbusSerialRtuClient::readInputContacts(int address, int number, std::vector<bool>& values)
{
    auto result = ModbusClient::readInputContacts(address, number, values);

    sleepBetweenModbusMessages();
    return result;
}

bool LibModbusSerialRtuClient::readHoldingRegister(int address, short& value)
{
    auto result = ModbusClient::readHoldingRegister(address, value);

    sleepBetweenModbusMessages();
    return result;
}

bool LibModbusSerialRtuClient::readHoldingRegisters(int address, int number, std::vector<signed short>& values)
{
    auto result = ModbusClient::readHoldingRegisters(address, number, values);

    sleepBetweenModbusMessages();
    return result;
}

bool LibModbusSerialRtuClient::readHoldingRegister(int address, unsigned short& value)
{
    auto result = ModbusClient::readHoldingRegister(address, value);

    sleepBetweenModbusMessages();
    return result;
}

bool LibModbusSerialRtuClient::readHoldingRegisters(int address, int number, std::vector<unsigned short>& values)
{
    auto result = ModbusClient::readHoldingRegisters(address, number, values);

    sleepBetweenModbusMessages();
    return result;
}

bool LibModbusSerialRtuClient::readHoldingRegister(int address, float& value)
{
    auto result = ModbusClient::readHoldingRegister(address, value);

    sleepBetweenModbusMessages();
    return result;
}

bool LibModbusSerialRtuClient::readHoldingRegisters(int address, int number, std::vector<float>& values)
{
    auto result = ModbusClient::readHoldingRegisters(address, number, values);

    sleepBetweenModbusMessages();
    return result;
}

bool LibModbusSerialRtuClient::readCoil(int address, bool& value)
{
    auto result = ModbusClient::readCoil(address, value);

    sleepBetweenModbusMessages();
    return result;
}

bool LibModbusSerialRtuClient::readCoils(int address, int number, std::vector<bool>& values)
{
    auto result = ModbusClient::readCoils(address, number, values);

    sleepBetweenModbusMessages();
    return result;
}

void LibModbusSerialRtuClient::sleepBetweenModbusMessages() const
{
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}
}    // namespace wolkabout
