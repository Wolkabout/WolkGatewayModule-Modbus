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

#ifndef MODULECONFIGURATION_H
#define MODULECONFIGURATION_H

#include "SerialRtuConfiguration.h"
#include "TcpIpConfiguration.h"
#include "utilities/json.hpp"

#include <chrono>
#include <memory>
#include <string>

namespace wolkabout
{
using nlohmann::json;

/**
 * @brief Model class containing information for assembling the moduleConfiguration.json file,
 *        one that sets up the module with the modbus connection, and connection to the MQTT
 *        broker, and some other miscellaneous things.
 */
class ModuleConfiguration
{
public:
    enum class ConnectionType
    {
        TCP_IP,
        SERIAL_RTU
    };

    ModuleConfiguration(std::string mqttHost, ConnectionType connectionType,
                        std::unique_ptr<SerialRtuConfiguration> serialRtuConfiguration,
                        std::chrono::milliseconds responseTimeout, std::chrono::milliseconds registerReadPeriod);

    ModuleConfiguration(std::string mqttHost, ConnectionType connectionType,
                        std::unique_ptr<TcpIpConfiguration> tcpIpConfiguration,
                        std::chrono::milliseconds responseTimeout, std::chrono::milliseconds registerReadPeriod);

    explicit ModuleConfiguration(nlohmann::json j);

    const std::string& getMqttHost() const;

    ModuleConfiguration::ConnectionType getConnectionType() const;

    const std::unique_ptr<SerialRtuConfiguration>& getSerialRtuConfiguration() const;

    const std::unique_ptr<TcpIpConfiguration>& getTcpIpConfiguration() const;

    const std::chrono::milliseconds& getResponseTimeout() const;

    const std::chrono::milliseconds& getRegisterReadPeriod() const;

    void setSerialRtuConfiguration(std::unique_ptr<SerialRtuConfiguration> serialRtuConfiguration);

    void setTcpIpConfiguration(std::unique_ptr<TcpIpConfiguration> tcpIpConfiguration);

private:
    std::string m_mqttHost;

    ConnectionType m_connectionType;

    std::unique_ptr<SerialRtuConfiguration> m_serialRtuConfiguration;
    std::unique_ptr<TcpIpConfiguration> m_tcpIpConfiguration;

    std::chrono::milliseconds m_responseTimeout;
    std::chrono::milliseconds m_registerReadPeriod;
};
}    // namespace wolkabout

#endif
