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

namespace wolkabout
{
ModbusClient::ModbusClient(std::chrono::milliseconds responseTimeout)
: m_responseTimeout(std::move(responseTimeout)), m_modbus(nullptr)
{
}

bool ModbusClient::connect()
{
    if (m_connected)
    {
        return true;
    }

    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!m_contextCreated && !createContext())
    {
        return false;
    }
    m_contextCreated = true;

    if (modbus_set_response_timeout(m_modbus, 2, 0) == -1)
    {
        LOG(ERROR) << "LibModbusClient: Unable to set response timeout - " << modbus_strerror(errno);
        destroyContext();
        return false;
    }

    if (modbus_connect(m_modbus) == -1)
    {
        LOG(ERROR) << "LibModbusClient: Unable to connect - " << modbus_strerror(errno);
        return false;
    }
    LOG(INFO) << "LibModbusClient: Connected successfully.";
    m_connected = true;

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
    modbus_flush(m_modbus);
    modbus_close(m_modbus);
    m_connected = false;
    return true;
}

bool ModbusClient::isConnected()
{
    LOG(DEBUG) << "Asking if connected : " << m_connected;
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    return m_connected;
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

bool ModbusClient::writeHoldingRegisters(int slaveAddress, int address, std::vector<short>& values)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return writeHoldingRegisters(address, values);
}

bool ModbusClient::writeHoldingRegisters(int slaveAddress, int address, std::vector<unsigned short>& values)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return writeHoldingRegisters(address, values);
}

bool ModbusClient::writeHoldingRegisters(int slaveAddress, int address, std::vector<float>& values)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return writeHoldingRegisters(address, values);
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

bool ModbusClient::readInputRegisters(int slaveAddress, int address, int number, std::vector<signed short>& values)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return readInputRegisters(address, number, values);
}

bool ModbusClient::readInputRegisters(int slaveAddress, int address, int number, std::vector<unsigned short>& values)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return readInputRegisters(address, number, values);
}

bool ModbusClient::readInputRegisters(int slaveAddress, int address, int number, std::vector<float>& values)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return readInputRegisters(address, number, values);
}

bool ModbusClient::readInputContacts(int slaveAddress, int address, int number, std::vector<bool>& values)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return readInputContacts(address, number, values);
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

bool ModbusClient::readHoldingRegisters(int slaveAddress, int address, int number, std::vector<signed short>& values)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return readHoldingRegisters(address, number, values);
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

bool ModbusClient::readHoldingRegisters(int slaveAddress, int address, int number, std::vector<unsigned short>& values)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return readHoldingRegisters(address, number, values);
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

bool ModbusClient::readHoldingRegisters(int slaveAddress, int address, int number, std::vector<float>& values)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }

    return readHoldingRegisters(address, number, values);
}

bool ModbusClient::readCoil(int slaveAddress, int address, bool& value)
{
    std::lock_guard<decltype(m_modbusMutex)> l{m_modbusMutex};
    if (!changeSlaveAddress(slaveAddress))
    {
        return false;
    }
    bool read = readCoil(address, value);
    return read;
}

bool ModbusClient::readCoils(int slaveAddress, int address, int number, std::vector<bool>& values)
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

bool ModbusClient::writeHoldingRegisters(int address, std::vector<short>& values)
{
    auto* arr = new uint16_t[values.size()];
    for (int i = 0; i < values.size(); i++)
    {
        ModbusValue modbusValue;
        modbusValue.signedShortValue = values[i];
        arr[i] = modbusValue.unsignedShortValue;
    }

    if (modbus_write_registers(m_modbus, address, values.size(), arr) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to write holding registers - " << modbus_strerror(errno);
        delete[](arr);
        return false;
    }

    delete[](arr);
    return true;
}

bool ModbusClient::writeHoldingRegisters(int address, std::vector<unsigned short>& values)
{
    auto* arr = new uint16_t[values.size()];
    for (int i = 0; i < values.size(); i++)
    {
        arr[i] = values[i];
    }

    if (modbus_write_registers(m_modbus, address, values.size(), arr) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to write holding registers - " << modbus_strerror(errno);
        delete[](arr);
        return false;
    }

    delete[](arr);
    return true;
}

bool ModbusClient::writeHoldingRegisters(int address, std::vector<float>& values)
{
    auto* arr = new uint16_t[values.size() * 2];
    for (int i = 0; i < values.size(); i++)
    {
        ModbusValue value;
        value.floatValue = values[i];
        arr[i * 2] = value.unsignedShortValues[0];
        arr[i * 1] = value.unsignedShortValues[1];
    }

    if (modbus_write_registers(m_modbus, address, values.size() * 2, arr) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to write holding registers - " << modbus_strerror(errno);
        delete[](arr);
        return false;
    }

    delete[](arr);
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

bool ModbusClient::readInputRegisters(int address, int number, std::vector<signed short>& values)
{
    std::vector<unsigned short> tmpValues(number);

    if (modbus_read_input_registers(m_modbus, address, number, &tmpValues[0]) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read input registers - " << modbus_strerror(errno);
        return false;
    }

    for (unsigned short& tmpValue : tmpValues)
    {
        values.push_back(reinterpret_cast<short&>(tmpValue));
    }
    return true;
}

bool ModbusClient::readInputRegisters(int address, int number, std::vector<unsigned short>& values)
{
    std::vector<unsigned short> tmpValues(number);

    if (modbus_read_input_registers(m_modbus, address, number, &tmpValues[0]) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read input registers - " << modbus_strerror(errno);
        return false;
    }

    for (const unsigned short& tmpValue : tmpValues)
    {
        values.push_back(tmpValue);
    }

    return true;
}

bool ModbusClient::readInputRegisters(int address, int number, std::vector<float>& values)
{
    std::vector<std::uint16_t> tmpValues(number * 2);

    if (modbus_read_input_registers(m_modbus, address, number * 2, &tmpValues[0]) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read input registers - " << modbus_strerror(errno);
        return false;
    }

    for (int i = 0; i < number * 2; i += 2)
    {
        float value;
        std::uint32_t tmpValue = tmpValues[i];
        tmpValue += tmpValues[i + 1] << 16;
        values.push_back(reinterpret_cast<float&>(tmpValue));
    }

    return true;
}

bool ModbusClient::readInputContacts(int address, int number, std::vector<bool>& values)
{
    std::vector<std::uint8_t> tmpValues(number / 8 + (number % 8 != 0));

    if (modbus_read_input_bits(m_modbus, address, number, &tmpValues[0]) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read input contacts - " << modbus_strerror(errno);
        return false;
    }

    for (unsigned short i = 0; i < tmpValues.size(); ++i)
    {
        int bits;
        if (i == tmpValues.size() - 1)
        {
            bits = number % 8;
            if (bits == 0)
            {
                bits = 8;
            }
        }
        else
        {
            bits = 8;
        }
        std::uint8_t mask = 1;
        for (int j = 0; j < bits; ++j)
        {
            std::uint8_t tmpValue = mask & tmpValues[i];
            values.push_back(static_cast<bool>(tmpValue));
            mask << 1;
        }
    }
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

bool ModbusClient::readHoldingRegisters(int address, int number, std::vector<signed short>& values)
{
    std::vector<unsigned short> tmpValues(number);

    if (modbus_read_registers(m_modbus, address, number, &tmpValues[0]) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read holding registers - " << modbus_strerror(errno);
        return false;
    }

    for (unsigned short& tmpValue : tmpValues)
    {
        values.push_back(reinterpret_cast<short&>(tmpValue));
    }
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

bool ModbusClient::readHoldingRegisters(int address, int number, std::vector<unsigned short>& values)
{
    std::vector<unsigned short> tmpValues(number);

    if (modbus_read_registers(m_modbus, address, number, &tmpValues[0]) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read holding registers - " << modbus_strerror(errno);
        return false;
    }

    for (const unsigned short& tmpValue : tmpValues)
    {
        values.push_back(tmpValue);
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

bool ModbusClient::readHoldingRegisters(int address, int number, std::vector<float>& values)
{
    std::vector<std::uint16_t> tmpValues(number * 2);

    if (modbus_read_registers(m_modbus, address, number * 2, &tmpValues[0]) == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read input registers - " << modbus_strerror(errno);
        return false;
    }

    for (int i = 0; i < number * 2; i += 2)
    {
        std::uint32_t tmpValue = tmpValues[i];
        tmpValue += tmpValues[i + 1] << 16;
        values.push_back(reinterpret_cast<float&>(tmpValue));
    }

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

bool ModbusClient::readCoils(int address, int number, std::vector<bool>& values)
{
    std::vector<std::uint8_t> tmpValues(number);
    int bits_read = 0;
    bits_read = modbus_read_bits(m_modbus, address, number, &tmpValues[0]);
    if (bits_read == -1)
    {
        LOG(DEBUG) << "LibModbusClient: Unable to read coils - " << modbus_strerror(errno);
        return false;
    }

    for (unsigned short i = 0; i < bits_read; i++)
    {
        values.push_back(static_cast<bool>(tmpValues[i]));
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
