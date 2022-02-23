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

#include "core/utilities/json.hpp"

#include <string>

namespace wolkabout
{
namespace modbus
{
using nlohmann::json;

/**
 * @brief Model class representing information necessary to create a TCP/IP connection.
 */
class TcpIpConfiguration
{
public:
    explicit TcpIpConfiguration(nlohmann::json j);

    TcpIpConfiguration(std::string ip, int port);

    const std::string& getIp() const;

    int getPort() const;

private:
    std::string m_ip;
    int m_port;
};
}    // namespace modbus
}    // namespace wolkabout
