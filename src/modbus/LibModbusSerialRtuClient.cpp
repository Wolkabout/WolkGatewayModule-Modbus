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

#include "modbus/LibModbusSerialRtuClient.h"
#include "modbus/ModbusClient.h"
#include "modbus/libmodbus/modbus.h"
#include "utilities/Logger.h"

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <utility>

namespace wolkabout
{
LibModbusSerialRtuClient::LibModbusSerialRtuClient(std::string serialPort, int baudRate, char dataBits, char stopBits,
                                                   ModbusConfiguration::BitParity bitParity, int slaveAddress,
                                                   std::chrono::milliseconds responseTimeout)
: m_serialPort(std::move(serialPort))
, m_baudRate(baudRate)
, m_dataBits(dataBits)
, m_stopBits(stopBits)
, m_slaveAddress(slaveAddress)
, m_responseTimeout(std::move(responseTimeout))
, m_modbus(nullptr)
{
    switch (bitParity)
    {
    case ModbusConfiguration::BitParity::NONE:
        m_bitParity = 'N';
        break;

    case ModbusConfiguration::BitParity::EVEN:
        m_bitParity = 'E';
        break;

    case ModbusConfiguration::BitParity::ODD:
        m_bitParity = 'O';
        break;
    }
}

LibModbusSerialRtuClient::~LibModbusSerialRtuClient()
{
    disconnect();
}

bool LibModbusSerialRtuClient::connect()
{
    LOG(INFO) << "LibModbusClient: Opening serial port '" << m_serialPort << "'  Baud: " << m_baudRate;

    std::lock_guard<decltype(m_modbusMutex)> l(m_modbusMutex);
    m_modbus = modbus_new_rtu(m_serialPort.c_str(), m_baudRate, m_bitParity, m_dataBits, m_stopBits);
    if (m_modbus == NULL)
    {
        LOG(ERROR) << "LibModbusClient: Unable to create modbus context - " << modbus_strerror(errno);
        return false;
    }

    if (modbus_set_response_timeout(m_modbus, 2, 0) == -1)
    {
        LOG(ERROR) << "LibModbusClient: Unable to set response timeout - " << modbus_strerror(errno);
        disconnect();
        return false;
    }

    if (modbus_connect(m_modbus) == -1)
    {
        LOG(ERROR) << "LibModbusClient: Unable to connect - " << modbus_strerror(errno);
        disconnect();
        return false;
    }

    auto responseTimeOutSeconds = std::chrono::duration_cast<std::chrono::seconds>(m_responseTimeout);
    auto responseTimeOutMicrosecondsResidue =
      std::chrono::duration_cast<std::chrono::microseconds>(m_responseTimeout) - responseTimeOutSeconds;

    if (modbus_set_response_timeout(m_modbus, static_cast<uint32_t>(responseTimeOutSeconds.count()),
                                    static_cast<uint32_t>(responseTimeOutMicrosecondsResidue.count())) == -1)
    {
        LOG(ERROR) << "LibModbusClient: Unable to set response timeout - " << modbus_strerror(errno);
        disconnect();
        return false;
    }

    if (modbus_set_slave(m_modbus, m_slaveAddress) == -1)
    {
        LOG(ERROR) << "LibModbusClient: Unable to set slave address - " << modbus_strerror(errno);
        disconnect();
        return false;
    }

    return true;
}

bool LibModbusSerialRtuClient::disconnect()
{
    if (m_modbus)
    {
        LOG(INFO) << "LibModbusClient: Closing serial port '" << m_serialPort << "'";

        std::lock_guard<decltype(m_modbusMutex)> l(m_modbusMutex);
        modbus_flush(m_modbus);
        modbus_close(m_modbus);
        modbus_free(m_modbus);
        m_modbus = nullptr;
    }

    return true;
}

bool LibModbusSerialRtuClient::isConnected()
{
    std::lock_guard<decltype(m_modbusMutex)> l(m_modbusMutex);
    return m_modbus != nullptr;
}

bool LibModbusSerialRtuClient::writeHoldingRegister(int address, signed short value)
{
    ModbusValue modbusValue;
    modbusValue.signedShortValue = value;

    std::lock_guard<decltype(m_modbusMutex)> l(m_modbusMutex);
    if (modbus_write_register(m_modbus, address, modbusValue.unsignedShortValue) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to write holding register - " << modbus_strerror(errno);
        return false;
    }

    return true;
}

bool LibModbusSerialRtuClient::writeHoldingRegister(int address, unsigned short value)
{
    std::lock_guard<decltype(m_modbusMutex)> l(m_modbusMutex);
    if (modbus_write_register(m_modbus, address, value) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to write holding register - " << modbus_strerror(errno);
        return false;
    }

    return true;
}

bool LibModbusSerialRtuClient::writeHoldingRegister(int address, float value)
{
    ModbusValue modbusValue;
    modbusValue.floatValue = value;

    std::lock_guard<decltype(m_modbusMutex)> l(m_modbusMutex);
    if (modbus_write_registers(m_modbus, address, 2, modbusValue.unsignedShortValues) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to write holding register - " << modbus_strerror(errno);
        return false;
    }

    return true;
}

bool LibModbusSerialRtuClient::writeCoil(int address, bool value)
{
    std::lock_guard<decltype(m_modbusMutex)> l(m_modbusMutex);
    if (modbus_write_bit(m_modbus, address, value ? TRUE : FALSE) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to write coil - " << modbus_strerror(errno);
        return false;
    }

    return true;
}

bool LibModbusSerialRtuClient::readInputRegister(int address, signed short& value)
{
    ModbusValue modbusValue;

    std::lock_guard<decltype(m_modbusMutex)> l(m_modbusMutex);
    if (modbus_read_input_registers(m_modbus, address, 1, &modbusValue.unsignedShortValue) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read input register - " << modbus_strerror(errno);
        return false;
    }

    value = modbusValue.signedShortValue;
    return true;
}

bool LibModbusSerialRtuClient::readInputRegister(int address, unsigned short& value)
{
    std::lock_guard<decltype(m_modbusMutex)> l(m_modbusMutex);
    if (modbus_read_input_registers(m_modbus, address, 1, &value) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read input register - " << modbus_strerror(errno);
        return false;
    }

    return true;
}

bool LibModbusSerialRtuClient::readInputRegister(int address, float& value)
{
    ModbusValue modbusValue;

    std::lock_guard<decltype(m_modbusMutex)> l(m_modbusMutex);
    if (modbus_read_input_registers(m_modbus, address, 2, modbusValue.unsignedShortValues) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read input register - " << modbus_strerror(errno);
        return false;
    }

    value = modbusValue.floatValue;
    return true;
}

bool LibModbusSerialRtuClient::readInputBit(int address, bool& value)
{
    uint8_t tmpValue = 0;

    std::lock_guard<decltype(m_modbusMutex)> l(m_modbusMutex);
    if (modbus_read_input_bits(m_modbus, address, 1, &tmpValue) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read input bit - " << modbus_strerror(errno);
        return false;
    }

    value = tmpValue != 0 ? true : false;
    return true;
}

bool LibModbusSerialRtuClient::readHoldingRegister(int address, short& value)
{
    ModbusValue modbusValue;

    std::lock_guard<decltype(m_modbusMutex)> l(m_modbusMutex);
    if (modbus_read_registers(m_modbus, address, 1, &modbusValue.unsignedShortValue) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read holding register - " << modbus_strerror(errno);
        return false;
    }

    value = modbusValue.signedShortValue;
    return true;
}

bool LibModbusSerialRtuClient::readHoldingRegister(int address, unsigned short& value)
{
    std::lock_guard<decltype(m_modbusMutex)> l(m_modbusMutex);
    if (modbus_read_registers(m_modbus, address, 1, &value) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read holding register - " << modbus_strerror(errno);
        return false;
    }

    return true;
}

bool LibModbusSerialRtuClient::readHoldingRegister(int address, float& value)
{
    ModbusValue modbusValue;

    std::lock_guard<decltype(m_modbusMutex)> l(m_modbusMutex);
    if (modbus_read_registers(m_modbus, address, 2, modbusValue.unsignedShortValues) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read holding register - " << modbus_strerror(errno);
        return false;
    }

    value = modbusValue.floatValue;
    return true;
}

bool LibModbusSerialRtuClient::readCoil(int address, bool& value)
{
    uint8_t tmpValue = 0;

    std::lock_guard<decltype(m_modbusMutex)> l(m_modbusMutex);
    if (modbus_read_bits(m_modbus, address, 1, &tmpValue) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read coil - " << modbus_strerror(errno);
        return false;
    }

    value = tmpValue != 0 ? true : false;
    return true;
}

bool LibModbusSerialRtuClient::changeSlaveAddress(int address)
{
    if (modbus_set_slave(m_modbus, address) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to set slave address - " << modbus_strerror(errno);
        return false;
    }

    return true;
}
}    // namespace wolkabout
