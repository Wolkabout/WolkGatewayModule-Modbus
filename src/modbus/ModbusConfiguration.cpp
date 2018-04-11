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

ModbusConfiguration::ModbusConfiguration(std::string ip, int port,
                                         std::chrono::duration<long long, std::milli> responseTimeout,
                                         std::chrono::duration<long long, std::milli> readPeriod)
: m_ip(std::move(ip)), m_port(port), m_responseTimeout(std::move(responseTimeout)), m_readPeriod(std::move(readPeriod))
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

const std::chrono::duration<long long, std::milli>& ModbusConfiguration::getResponseTimeout() const
{
    return m_responseTimeout;
}

const std::chrono::duration<long long, std::milli>& ModbusConfiguration::getReadPeriod() const
{
    return m_readPeriod;
}

wolkabout::ModbusConfiguration ModbusConfiguration::fromJson(const std::string& modbusConfigurationFile)
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
    const auto responseTimeout = j.at("responseTimeoutMs").get<long long>();
    const auto readPeriod = j.at("registerReadPeriodMs").get<long long>();

    return ModbusConfiguration(ip, port, std::chrono::duration<long long, std::milli>(responseTimeout),
                               std::chrono::duration<long long, std::milli>(readPeriod));
}
}    // namespace wolkabout
