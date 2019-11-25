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

#include "modbus/LibModbusTcpIpClient.h"
#include "modbus/ModbusClient.h"
#include "modbus/libmodbus/modbus.h"
#include "utilities/Logger.h"

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <utility>

namespace wolkabout
{
LibModbusTcpIpClient::LibModbusTcpIpClient(std::string ipAddress, int port, std::chrono::milliseconds responseTimeout)
: ModbusClient(std::move(responseTimeout)), m_ipAddress(std::move(ipAddress)), m_port(port)
{
}

LibModbusTcpIpClient::~LibModbusTcpIpClient()
{
    destroyContext();
}

bool LibModbusTcpIpClient::createContext()
{
    LOG(INFO) << "LibModbusClient: Connecting to " << m_ipAddress << ":" << m_port;

    m_modbus = modbus_new_tcp(nullptr, m_port);
    if (m_modbus == nullptr)
    {
        LOG(ERROR) << "LibModbusClient: Unable to create modbus context - " << modbus_strerror(errno);
        return false;
    }

    m_contextCreated = true;
    return true;
}

bool LibModbusTcpIpClient::destroyContext()
{
    if (m_modbus)
    {
        LOG(INFO) << "LibModbusClient: Disconnecting from " << m_ipAddress << ":" << m_port;

        std::lock_guard<decltype(m_modbusMutex)> l(m_modbusMutex);
        disconnect();
        modbus_free(m_modbus);
        m_modbus = nullptr;
    }

    return true;
}

}    // namespace wolkabout
