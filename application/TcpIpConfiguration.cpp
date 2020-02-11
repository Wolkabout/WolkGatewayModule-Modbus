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

#include "TcpIpConfiguration.h"

namespace wolkabout
{
TcpIpConfiguration::TcpIpConfiguration(std::string ip, int port) : m_ip(ip), m_port(port) {}

const std::string& TcpIpConfiguration::getIp() const
{
    return m_ip;
}

int TcpIpConfiguration::getPort() const
{
    return m_port;
}
}    // namespace wolkabout
