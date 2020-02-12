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

#include "ModuleConfiguration.h"
#include "utilities/FileSystemUtils.h"
#include "utilities/json.hpp"

#include <iostream>

namespace wolkabout
{
using nlohmann::json;

ModuleConfiguration::ModuleConfiguration(std::string mqttHost, ConnectionType connectionType,
                                         std::unique_ptr<SerialRtuConfiguration> serialRtuConfiguration,
                                         std::chrono::milliseconds responseTimeout,
                                         std::chrono::milliseconds registerReadPeriod)
: m_mqttHost(std::move(mqttHost))
, m_connectionType(connectionType)
, m_serialRtuConfiguration(std::move(serialRtuConfiguration))
, m_tcpIpConfiguration(nullptr)
, m_responseTimeout(responseTimeout)
, m_registerReadPeriod(registerReadPeriod)
{
}

ModuleConfiguration::ModuleConfiguration(std::string mqttHost, ConnectionType connectionType,
                                         std::unique_ptr<TcpIpConfiguration> tcpIpConfiguration,
                                         std::chrono::milliseconds responseTimeout,
                                         std::chrono::milliseconds registerReadPeriod)
: m_mqttHost(std::move(mqttHost))
, m_connectionType(connectionType)
, m_serialRtuConfiguration(nullptr)
, m_tcpIpConfiguration(std::move(tcpIpConfiguration))
, m_responseTimeout(responseTimeout)
, m_registerReadPeriod(registerReadPeriod)
{
}

const std::string& ModuleConfiguration::getMqttHost() const
{
    return m_mqttHost;
}

ModuleConfiguration::ConnectionType ModuleConfiguration::getConnectionType() const
{
    return m_connectionType;
}

std::unique_ptr<SerialRtuConfiguration> ModuleConfiguration::getSerialRtuConfiguration()
{
    return std::move(m_serialRtuConfiguration);
}

std::unique_ptr<TcpIpConfiguration> ModuleConfiguration::getTcpIpConfiguration()
{
    return std::move(m_tcpIpConfiguration);
}

const std::chrono::milliseconds& ModuleConfiguration::getResponseTimeout() const
{
    return m_responseTimeout;
}

const std::chrono::milliseconds& ModuleConfiguration::getRegisterReadPeriod() const
{
    return m_registerReadPeriod;
}

void ModuleConfiguration::setSerialRtuConfiguration(std::unique_ptr<SerialRtuConfiguration> serialRtuConfiguration)
{
    m_serialRtuConfiguration = std::move(serialRtuConfiguration);
}

void ModuleConfiguration::setTcpIpConfiguration(std::unique_ptr<TcpIpConfiguration> tcpIpConfiguration)
{
    m_tcpIpConfiguration = std::move(tcpIpConfiguration);
}

wolkabout::ModuleConfiguration ModuleConfiguration::fromJsonFile(const std::string& moduleConfigurationFile)
{
    if (!FileSystemUtils::isFilePresent(moduleConfigurationFile))
    {
        throw std::logic_error("Given module configuration file does not exist.");
    }

    std::string moduleConfigurationJson;
    if (!FileSystemUtils::readFileContent(moduleConfigurationFile, moduleConfigurationJson))
    {
        throw std::logic_error("Unable to read module configuration file.");
    }

    auto j = json::parse(moduleConfigurationJson);
    std::string mqttHost;
    try
    {
        mqttHost = j.at("mqttHost").get<std::string>();
    }
    catch (std::exception&)
    {
        mqttHost = "tcp://localhost:1883";
    }

    std::string connectionTypeStr;
    try
    {
        connectionTypeStr = j.at("connectionType").get<std::string>();
    }
    catch (std::exception&)
    {
        throw std::logic_error("Missing configuration field : connectionType");
    }

    std::unique_ptr<SerialRtuConfiguration> serialRtuConfig;
    std::unique_ptr<TcpIpConfiguration> tcpIpConfig;

    const auto connectionType = [&]() -> ConnectionType {
        if (connectionTypeStr == "TCP/IP")
        {
            tcpIpConfig = std::unique_ptr<TcpIpConfiguration>(new TcpIpConfiguration(j["tcp/ip"]));
            return ConnectionType::TCP_IP;
        }
        else if (connectionTypeStr == "SERIAL/RTU")
        {
            serialRtuConfig = std::unique_ptr<SerialRtuConfiguration>(new SerialRtuConfiguration(j["serial/rtu"]));
            return ConnectionType::SERIAL_RTU;
        }
        throw std::logic_error("Unknown modbus connection type : " + connectionTypeStr);
    }();

    long long responseTimeout;
    try
    {
        responseTimeout = j.at("responseTimeoutMs").get<long long>();
    }
    catch (std::exception&)
    {
        responseTimeout = 200;
    }

    long long registerReadPeriod;
    try
    {
        registerReadPeriod = j.at("registerReadPeriodMs").get<long long>();
    }
    catch (std::exception&)
    {
        registerReadPeriod = 500;
    }

    if (connectionTypeStr == "SERIAL/RTU")
    {
        return ModuleConfiguration(mqttHost, connectionType, std::move(serialRtuConfig),
                                   std::chrono::milliseconds(responseTimeout),
                                   std::chrono::milliseconds(registerReadPeriod));
    }
    else
    {
        return ModuleConfiguration(mqttHost, connectionType, std::move(tcpIpConfig),
                                   std::chrono::milliseconds(responseTimeout),
                                   std::chrono::milliseconds(registerReadPeriod));
    }
}
}    // namespace wolkabout
