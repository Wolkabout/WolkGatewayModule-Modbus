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

#ifndef MODBUSCONFIGURATION_H
#define MODBUSCONFIGURATION_H

#include <chrono>
#include <string>

namespace wolkabout
{
class ModbusConfiguration
{
public:
    enum class ConnectionType
    {
        TCP_IP,
        SERIAL_RTU
    };

    enum class BitParity
    {
        NONE,
        EVEN,
        ODD
    };

    ModbusConfiguration() = default;

    ModbusConfiguration(std::string ip, int port, std::string serialPort, int baudRate, char dataBits, char stopBits,
                        ModbusConfiguration::BitParity bitParity, ModbusConfiguration::ConnectionType connectionType,
                        std::chrono::milliseconds responseTimeout, std::chrono::milliseconds readPeriod);

    const std::string& getIp() const;
    int getPort() const;

    const std::string& getSerialPort() const;
    int getBaudRate() const;
    char getDataBits() const;
    char getStopBits() const;
    ModbusConfiguration::BitParity getBitParity() const;

    ModbusConfiguration::ConnectionType getConnectionType() const;

    const std::chrono::milliseconds& getResponseTimeout() const;

    const std::chrono::milliseconds& getReadPeriod() const;

    static wolkabout::ModbusConfiguration fromJsonFile(const std::string& modbusConfigurationFile);

private:
    std::string m_ip;
    int m_port;

    std::string m_serialPort;
    int m_baudRate;
    char m_dataBits;
    char m_stopBits;
    BitParity m_bitParity;

    ConnectionType m_connectionType;

    std::chrono::milliseconds m_responseTimeout;
    std::chrono::milliseconds m_readPeriod;
};
}    // namespace wolkabout

#endif
