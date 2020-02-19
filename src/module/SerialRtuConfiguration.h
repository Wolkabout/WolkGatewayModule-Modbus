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
#include "utilities/json.hpp"

#include <string>

namespace wolkabout
{
using nlohmann::json;

class SerialRtuConfiguration
{
public:
    enum class BitParity
    {
        NONE,
        EVEN,
        ODD
    };

    SerialRtuConfiguration() = default;

    SerialRtuConfiguration(std::string& serialPort, int baudRate, char dataBits, char stopBits,
                           SerialRtuConfiguration::BitParity bitParity);

    explicit SerialRtuConfiguration(nlohmann::json j);

    const std::string& getSerialPort() const;

    int getBaudRate() const;

    char getDataBits() const;

    char getStopBits() const;

    SerialRtuConfiguration::BitParity getBitParity() const;

private:
    std::string m_serialPort;
    int m_baudRate;
    char m_dataBits;
    char m_stopBits;
    SerialRtuConfiguration::BitParity m_bitParity;
};
}    // namespace wolkabout
