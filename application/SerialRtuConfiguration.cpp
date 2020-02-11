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

#include "SerialRtuConfiguration.h"

namespace wolkabout
{
SerialRtuConfiguration::SerialRtuConfiguration(std::string& serialPort, uint baudRate, short dataBits, short stopBits,
                                               SerialRtuConfiguration::BitParity bitParity)
: m_serialPort(serialPort), m_baudRate(baudRate), m_dataBits(dataBits), m_stopBits(stopBits), m_bitParity(bitParity)
{
}

const std::string& SerialRtuConfiguration::getSerialPort() const
{
    return m_serialPort;
}

const uint SerialRtuConfiguration::getBaudRate() const
{
    return m_baudRate;
}

const short SerialRtuConfiguration::getDataBits() const
{
    return m_dataBits;
}

const short SerialRtuConfiguration::getStopBits() const
{
    return m_stopBits;
}

SerialRtuConfiguration::BitParity SerialRtuConfiguration::getBitParity() const
{
    return m_bitParity;
}
}    // namespace wolkabout
