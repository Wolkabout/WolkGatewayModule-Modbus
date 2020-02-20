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

#include "DeviceInformation.h"

#include <utility>

namespace wolkabout
{
wolkabout::DeviceInformation::DeviceInformation(std::string& name, std::string& key,
                                                std::unique_ptr<DevicesConfigurationTemplate>* deviceTemplate,
                                                int slaveAddress)
: m_name(name), m_key(key), m_slaveAddress(slaveAddress), m_templateString(), m_template(deviceTemplate)
{
}

DeviceInformation::DeviceInformation(nlohmann::json j) : m_template(nullptr)
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

    try
    {
        m_slaveAddress = j["slaveAddress"].get<int>();
    }
    catch (std::exception&)
    {
        m_slaveAddress = -1;
    }
}

DeviceInformation::DeviceInformation(nlohmann::json j, std::unique_ptr<DevicesConfigurationTemplate>* templatePointer)
: DeviceInformation(std::move(j))
{
    m_template = templatePointer;
}

DeviceInformation::DeviceInformation(const DeviceInformation& instance)
: m_name(instance.getName())
, m_key(instance.getKey())
, m_slaveAddress(instance.getSlaveAddress())
, m_templateString(instance.getTemplateString())
, m_template(instance.getTemplate())
{
}

const std::string& DeviceInformation::getName() const
{
    return m_name;
}

const std::string& DeviceInformation::getKey() const
{
    return m_key;
}

int DeviceInformation::getSlaveAddress() const
{
    return m_slaveAddress;
}

void DeviceInformation::setSlaveAddress(int slaveAddress)
{
    m_slaveAddress = slaveAddress;
}

const std::string& DeviceInformation::getTemplateString() const
{
    return m_templateString;
}

std::unique_ptr<DevicesConfigurationTemplate>* DeviceInformation::getTemplate() const
{
    return m_template;
}

void DeviceInformation::setTemplate(std::unique_ptr<DevicesConfigurationTemplate>* templatePointer)
{
    m_template = templatePointer;
}
}    // namespace wolkabout
