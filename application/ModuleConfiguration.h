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

#include <chrono>
#include <string>

namespace wolkabout
{
class ModuleConfiguration
{
public:
    enum class ConnectionType
    {
        TCP_IP,
        SERIAL_RTU
    };

    ModuleConfiguration() = default;

    ModuleConfiguration(std::string mqttHost, ConnectionType connectionType, std::chrono::milliseconds responseTimeout,
                        std::chrono::milliseconds registerReadPeriod);

    const std::string& getMqttHost() const;

    ModuleConfiguration::ConnectionType getConnectionType() const;

    const std::chrono::milliseconds& getResponseTimeout() const;

    const std::chrono::milliseconds& getRegisterReadPeriod() const;

    static wolkabout::ModuleConfiguration fromJsonFile(const std::string& moduleConfigurationFile);

private:
    std::string m_mqttHost;

    ConnectionType m_connectionType;

    std::chrono::milliseconds m_responseTimeout;
    std::chrono::milliseconds m_registerReadPeriod;
};
}    // namespace wolkabout

#endif
