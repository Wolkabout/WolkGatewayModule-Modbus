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
#include "modbus/LibModbusSerialRtuClient.h"

#include <string>

namespace wolkabout
{
using nlohmann::json;

/**
 * @brief Model class representing information necessary to create a SERIAL/RTU connection.
 */
class SerialRtuConfiguration
{
public:
    SerialRtuConfiguration(std::string& serialPort, int baudRate, char dataBits, char stopBits,
                           LibModbusSerialRtuClient::BitParity bitParity);

    explicit SerialRtuConfiguration(nlohmann::json j);

    const std::string& getSerialPort() const;

    int getBaudRate() const;

    char getDataBits() const;

    char getStopBits() const;

    LibModbusSerialRtuClient::BitParity getBitParity() const;

private:
    std::string m_serialPort;
    int m_baudRate;
    char m_dataBits;
    char m_stopBits;
    LibModbusSerialRtuClient::BitParity m_bitParity;
};
}    // namespace wolkabout
