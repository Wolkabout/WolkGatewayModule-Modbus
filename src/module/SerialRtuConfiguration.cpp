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

#include "SerialRtuConfiguration.h"
#include "utilities/FileSystemUtils.h"

namespace wolkabout
{
SerialRtuConfiguration::SerialRtuConfiguration(std::string& serialPort, int baudRate, char dataBits, char stopBits,
                                               LibModbusSerialRtuClient::BitParity bitParity)
: m_serialPort(serialPort), m_baudRate(baudRate), m_dataBits(dataBits), m_stopBits(stopBits), m_bitParity(bitParity)
{
}

SerialRtuConfiguration::SerialRtuConfiguration(nlohmann::json j)
{
    try
    {
        m_serialPort = j.at("serialPort").get<std::string>();
    }
    catch (std::exception&)
    {
        throw std::logic_error("Missing configuration field : serialPort");
    }

    try
    {
        m_baudRate = j.at("baudRate").get<int>();
    }
    catch (std::exception&)
    {
        m_baudRate = 115200;
    }

    try
    {
        m_dataBits = j.at("dataBits").get<char>();
    }
    catch (std::exception&)
    {
        m_dataBits = 8;
    }

    try
    {
        m_stopBits = j.at("stopBits").get<char>();
    }
    catch (std::exception&)
    {
        m_stopBits = 1;
    }

    try
    {
        std::string bitParityStr = j.at("bitParity").get<std::string>();
        m_bitParity = [&]() -> LibModbusSerialRtuClient::BitParity {
            if (bitParityStr == "NONE")
            {
                return LibModbusSerialRtuClient::BitParity::NONE;
            }
            else if (bitParityStr == "EVEN")
            {
                return LibModbusSerialRtuClient::BitParity::EVEN;
            }
            else if (bitParityStr == "ODD")
            {
                return LibModbusSerialRtuClient::BitParity::ODD;
            }
            throw std::logic_error("Unknown bit parity: " + bitParityStr);
        }();
    }
    catch (std::exception&)
    {
        m_bitParity = LibModbusSerialRtuClient::BitParity::NONE;
    }
}

const std::string& SerialRtuConfiguration::getSerialPort() const
{
    return m_serialPort;
}

int SerialRtuConfiguration::getBaudRate() const
{
    return m_baudRate;
}

char SerialRtuConfiguration::getDataBits() const
{
    return m_dataBits;
}

char SerialRtuConfiguration::getStopBits() const
{
    return m_stopBits;
}

LibModbusSerialRtuClient::BitParity SerialRtuConfiguration::getBitParity() const
{
    return m_bitParity;
}
}    // namespace wolkabout
