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

#include "modbus/model/DeviceInformation.h"

#include <utility>

namespace wolkabout
{
namespace modbus
{
DeviceInformation::DeviceInformation(std::string name, std::string key, const DeviceTemplate& deviceTemplate,
                                     std::uint16_t slaveAddress)
: m_name(std::move(name))
, m_key(std::move(key))
, m_slaveAddress(slaveAddress)
, m_templateString()
, m_template(deviceTemplate)
{
}

DeviceInformation::DeviceInformation(nlohmann::json j, const DeviceTemplate& deviceTemplate)
: m_template(deviceTemplate)
{
    try
    {
        m_name = j["name"].get<std::string>();
    }
    catch (std::exception&)
    {
        throw std::logic_error("Missing device information field - name.");
    }

    try
    {
        m_key = j["key"].get<std::string>();
    }
    catch (std::exception&)
    {
        throw std::logic_error("Missing device information field - key.");
    }

    // The device template really has to be set outside the class, but we can set the string
    try
    {
        m_templateString = j["template"].get<std::string>();
    }
    catch (std::exception&)
    {
        throw std::logic_error("Missing device information field - template name.");
    }
    if (m_templateString != m_template.getName())
    {
        throw std::logic_error("The template passed to the Device is not the same as the one listed by name.");
    }

    try
    {
        m_slaveAddress = j["slaveAddress"].get<std::uint16_t>();
    }
    catch (std::exception&)
    {
        m_slaveAddress = static_cast<std::uint16_t>(0);
    }
}

const std::string& DeviceInformation::getName() const
{
    return m_name;
}

const std::string& DeviceInformation::getKey() const
{
    return m_key;
}

std::uint16_t DeviceInformation::getSlaveAddress() const
{
    return m_slaveAddress;
}

void DeviceInformation::setSlaveAddress(std::uint16_t slaveAddress)
{
    m_slaveAddress = slaveAddress;
}

const std::string& DeviceInformation::getTemplateString() const
{
    return m_templateString;
}

const DeviceTemplate& DeviceInformation::getTemplate() const
{
    return m_template;
}
}    // namespace modbus
}    // namespace wolkabout
