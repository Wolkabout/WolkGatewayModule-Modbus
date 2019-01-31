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

#include "ModbusClient.h"
#include "modbus/libmodbus/modbus.h"
#include "utilities/Logger.h"

#include <cmath>

namespace wolkabout
{
ModbusClient::ModbusClient(std::chrono::milliseconds responseTimeout)
: m_responseTimeout(std::move(responseTimeout)), m_modbus(nullptr)
{
}

bool ModbusClient::connect()
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!createContext())
    {
        return false;
    }

    if (modbus_set_response_timeout(m_modbus, 2, 0) == -1)
    {
        LOG(ERROR) << "LibModbusClient: Unable to set response timeout - " << modbus_strerror(errno);
        destroyContext();
        return false;
    }

    if (modbus_connect(m_modbus) == -1)
    {
        LOG(ERROR) << "LibModbusClient: Unable to connect - " << modbus_strerror(errno);
        destroyContext();
        return false;
    }

    auto responseTimeOutSeconds = std::chrono::duration_cast<std::chrono::seconds>(m_responseTimeout);
    auto responseTimeOutMicrosecondsResidue =
      std::chrono::duration_cast<std::chrono::microseconds>(m_responseTimeout) - responseTimeOutSeconds;

    if (modbus_set_response_timeout(m_modbus, static_cast<uint32_t>(responseTimeOutSeconds.count()),
                                    static_cast<uint32_t>(responseTimeOutMicrosecondsResidue.count())) == -1)
    {
        LOG(ERROR) << "LibModbusClient: Unable to set response timeout - " << modbus_strerror(errno);
        destroyContext();
        return false;
    }

    return true;
}

bool ModbusClient::disconnect()
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    return destroyContext();
}

bool ModbusClient::isConnected()
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    return m_modbus != nullptr;
}

bool ModbusClient::writeHoldingRegister(int slaveAddress, int address, signed short value)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return writeHoldingRegister(address, value);
}

bool ModbusClient::writeHoldingRegister(int slaveAddress, int address, unsigned short value)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return writeHoldingRegister(address, value);
}

bool ModbusClient::writeHoldingRegister(int slaveAddress, int address, float value)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return writeHoldingRegister(address, value);
}

bool ModbusClient::writeCoil(int slaveAddress, int address, bool value)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return writeCoil(address, value);
}

bool ModbusClient::readInputRegister(int slaveAddress, int address, signed short& value)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return readInputRegister(address, value);
}

bool ModbusClient::readInputRegister(int slaveAddress, int address, unsigned short& value)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return readInputRegister(address, value);
}

bool ModbusClient::readInputRegister(int slaveAddress, int address, float& value)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return readInputRegister(address, value);
}

bool ModbusClient::readInputContact(int slaveAddress, int address, bool& value)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return readInputContact(address, value);
}

bool ModbusClient::readHoldingRegister(int slaveAddress, int address, signed short& value)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return readHoldingRegister(address, value);
}

bool ModbusClient::readHoldingRegister(int slaveAddress, int address, unsigned short& value)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return readHoldingRegister(address, value);
}

bool ModbusClient::readHoldingRegister(int slaveAddress, int address, float& value)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return readHoldingRegister(address, value);
}

bool ModbusClient::readCoil(int slaveAddress, int address, bool& value)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return readCoil(address, value);
}

bool ModbusClient::readCoils(int slaveAddress, int address, int number, std::vector<bool> values)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return readCoils(address, number, values);
}

//

bool ModbusClient::writeHoldingRegister(int address, signed short value)
{
    ModbusValue modbusValue;
    modbusValue.signedShortValue = value;

    if (modbus_write_register(m_modbus, address, modbusValue.unsignedShortValue) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to write holding register - " << modbus_strerror(errno);
        return false;
    }

    return true;
}

bool ModbusClient::writeHoldingRegister(int address, unsigned short value)
{
    if (modbus_write_register(m_modbus, address, value) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to write holding register - " << modbus_strerror(errno);
        return false;
    }

    return true;
}

bool ModbusClient::writeHoldingRegister(int address, float value)
{
    ModbusValue modbusValue;
    modbusValue.floatValue = value;

    if (modbus_write_registers(m_modbus, address, 2, modbusValue.unsignedShortValues) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to write holding register - " << modbus_strerror(errno);
        return false;
    }

    return true;
}

bool ModbusClient::writeCoil(int address, bool value)
{
    if (modbus_write_bit(m_modbus, address, value ? TRUE : FALSE) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to write coil - " << modbus_strerror(errno);
        return false;
    }

    return true;
}

bool ModbusClient::readInputRegister(int address, signed short& value)
{
    ModbusValue modbusValue;

    if (modbus_read_input_registers(m_modbus, address, 1, &modbusValue.unsignedShortValue) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read input register - " << modbus_strerror(errno);
        return false;
    }

    value = modbusValue.signedShortValue;
    return true;
}

bool ModbusClient::readInputRegister(int address, unsigned short& value)
{
    if (modbus_read_input_registers(m_modbus, address, 1, &value) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read input register - " << modbus_strerror(errno);
        return false;
    }

    return true;
}

bool ModbusClient::readInputRegister(int address, float& value)
{
    ModbusValue modbusValue;

    if (modbus_read_input_registers(m_modbus, address, 2, modbusValue.unsignedShortValues) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read input register - " << modbus_strerror(errno);
        return false;
    }

    value = modbusValue.floatValue;
    return true;
}

bool ModbusClient::readInputContact(int address, bool& value)
{
    uint8_t tmpValue = 0;

    if (modbus_read_input_bits(m_modbus, address, 1, &tmpValue) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read input contact - " << modbus_strerror(errno);
        return false;
    }

    value = tmpValue != 0 ? true : false;
    return true;
}

bool ModbusClient::readHoldingRegister(int address, short& value)
{
    ModbusValue modbusValue;

    if (modbus_read_registers(m_modbus, address, 1, &modbusValue.unsignedShortValue) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read holding register - " << modbus_strerror(errno);
        return false;
    }

    value = modbusValue.signedShortValue;
    return true;
}

bool ModbusClient::readHoldingRegister(int address, unsigned short& value)
{
    if (modbus_read_registers(m_modbus, address, 1, &value) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read holding register - " << modbus_strerror(errno);
        return false;
    }

    return true;
}

bool ModbusClient::readHoldingRegister(int address, float& value)
{
    ModbusValue modbusValue;

    if (modbus_read_registers(m_modbus, address, 2, modbusValue.unsignedShortValues) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read holding register - " << modbus_strerror(errno);
        return false;
    }

    value = modbusValue.floatValue;
    return true;
}

bool ModbusClient::readCoil(int address, bool& value)
{
    std::uint8_t tmpValue = 0;

    if (modbus_read_bits(m_modbus, address, 1, &tmpValue) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read coil - " << modbus_strerror(errno);
        return false;
    }

    value = tmpValue != 0 ? true : false;
    return true;
}

bool ModbusClient::readCoils(int address, int number, std::vector<bool> values)
{
    std::vector<std::uint8_t> tmpValues(number / 8 + (number % 8 != 0));

    if (modbus_read_bits(m_modbus, address, number, &tmpValues[0]) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read coils - " << modbus_strerror(errno);
        return false;
    }

    for (auto tmpValuesByte = tmpValues.begin(); tmpValuesByte != tmpValues.end(); ++tmpValuesByte)
    {
        std::uint8_t bitmask;
        if (tmpValuesByte != tmpValues.end())
        {
            bitmask = 128;
        }
        else
        {
            int bits = number % 8;
            if (bits == 0)
            {
                bitmask = 128;
            }
            else
            {
                bitmask = static_cast<unsigned char>(std::pow(2, bits - 1));
            }
        }
        for (std::uint8_t mask = 1; mask < bitmask;)
        {
            std::uint8_t tmpValue = mask & *tmpValuesByte;
            bool value = tmpValue != 0 ? true : false;
            values.push_back(value);
            if (mask == bitmask)
                break;
            else
                mask << 1;
        }
    }
    return true;
}

bool ModbusClient::changeSlaveAddress(int address)
{
    if (modbus_set_slave(m_modbus, address) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to set slave address - " << modbus_strerror(errno);
        return false;
    }

    return true;
}
}    // namespace wolkabout
