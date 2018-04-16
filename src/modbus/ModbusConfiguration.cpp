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

#include "ModbusConfiguration.h"
#include "utilities/FileSystemUtils.h"
#include "utilities/json.hpp"

#include <chrono>
#include <stdexcept>
#include <string>
#include <utility>

namespace wolkabout
{
using nlohmann::json;

ModbusConfiguration::ModbusConfiguration(std::string ip, int port, std::string serialPort, int baudRate, char dataBits,
                                         char stopBits, ModbusConfiguration::BitParity bitParity,
                                         ModbusConfiguration::ConnectionType connectionType,
                                         std::chrono::milliseconds responseTimeout,
                                         std::chrono::milliseconds readPeriod)
: m_ip(std::move(ip))
, m_port(port)
, m_serialPort(std::move(serialPort))
, m_baudRate(baudRate)
, m_dataBits(dataBits)
, m_stopBits(stopBits)
, m_bitParity(bitParity)
, m_connectionType(connectionType)
, m_responseTimeout(std::move(responseTimeout))
, m_readPeriod(std::move(readPeriod))
{
}

const std::string& ModbusConfiguration::getIp() const
{
    return m_ip;
}

int ModbusConfiguration::getPort() const
{
    return m_port;
}

const std::string& ModbusConfiguration::getSerialPort() const
{
    return m_serialPort;
}

int ModbusConfiguration::getBaudRate() const
{
    return m_baudRate;
}

char ModbusConfiguration::getDataBits() const
{
    return m_dataBits;
}

char ModbusConfiguration::getStopBits() const
{
    return m_stopBits;
}

ModbusConfiguration::BitParity ModbusConfiguration::getBitParity() const
{
    return m_bitParity;
}

ModbusConfiguration::ConnectionType ModbusConfiguration::getConnectionType() const
{
    return m_connectionType;
}

const std::chrono::milliseconds& ModbusConfiguration::getResponseTimeout() const
{
    return m_responseTimeout;
}

const std::chrono::milliseconds& ModbusConfiguration::getReadPeriod() const
{
    return m_readPeriod;
}

wolkabout::ModbusConfiguration ModbusConfiguration::fromJsonFile(const std::string& modbusConfigurationFile)
{
    if (!FileSystemUtils::isFilePresent(modbusConfigurationFile))
    {
        throw std::logic_error("Given modbus configuration file does not exist.");
    }

    std::string modbusConfigurationJson;
    if (!FileSystemUtils::readFileContent(modbusConfigurationFile, modbusConfigurationJson))
    {
        throw std::logic_error("Unable to read modbus configuration file.");
    }

    auto j = json::parse(modbusConfigurationJson);
    const auto ip = j.at("ip").get<std::string>();
    const auto port = j.at("port").get<int>();

    const auto serialPort = j.at("serialPort").get<std::string>();
    const auto baudRate = j.at("baudRate").get<int>();
    const auto dataBits = j.at("dataBits").get<char>();
    const auto stopBits = j.at("stopBits").get<char>();
    const auto bitParityStr = j.at("bitParity").get<std::string>();
    const auto bitParity = [&]() -> BitParity {
        if (bitParityStr == "NONE")
        {
            return BitParity::NONE;
        }
        else if (bitParityStr == "EVEN")
        {
            return BitParity::EVEN;
        }
        else if (bitParityStr == "ODD")
        {
            return BitParity::ODD;
        }

        throw std::logic_error("Unknown bit parity: " + bitParityStr);
    }();

    const auto connectionTypeStr = j.at("connectionType").get<std::string>();
    const auto connectionType = [&]() -> ConnectionType {
        if (connectionTypeStr == "TCP/IP")
        {
            return ConnectionType::TCP_IP;
        }
        else if (connectionTypeStr == "SERIAL/RTU")
        {
            return ConnectionType::SERIAL_RTU;
        }

        throw std::logic_error("Unknown modbus connection type : " + connectionTypeStr);
    }();

    const auto responseTimeout = j.at("responseTimeoutMs").get<long long>();
    const auto readPeriod = j.at("registerReadPeriodMs").get<long long>();

    return ModbusConfiguration(ip, port, serialPort, baudRate, dataBits, stopBits, bitParity, connectionType,
                               std::chrono::milliseconds(responseTimeout), std::chrono::milliseconds(readPeriod));
}
}    // namespace wolkabout
