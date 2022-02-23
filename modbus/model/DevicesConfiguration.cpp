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

#include "modbus/model/DevicesConfiguration.h"

namespace wolkabout
{
namespace modbus
{
DevicesConfiguration::DevicesConfiguration(nlohmann::json j) : m_templates(), m_devices()
{
    for (json::object_t templateJson : j["templates"].get<json::array_t>())
    {
        std::string templateName = templateJson["name"].get<std::string>();
        m_templates.emplace(templateName, std::unique_ptr<DeviceTemplate>(new DeviceTemplate(templateJson)));
    }

    for (json::object_t deviceJson : j["devices"].get<json::array_t>())
    {
        std::string keyName = deviceJson["key"].get<std::string>();
        std::string templateName = deviceJson["template"].get<std::string>();

        if (m_templates.find(templateName) != m_templates.end())
            m_devices.emplace(keyName, std::unique_ptr<DeviceInformation>(
                                         new DeviceInformation{deviceJson, *m_templates[templateName]}));
        else
            throw std::logic_error("Missing template " + templateName + " required by device " + keyName + ".");
    }
}

const std::map<std::string, std::unique_ptr<DeviceTemplate>>& DevicesConfiguration::getTemplates() const
{
    return m_templates;
}

const std::map<std::string, std::unique_ptr<DeviceInformation>>& DevicesConfiguration::getDevices() const
{
    return m_devices;
}
}    // namespace modbus
}    // namespace wolkabout
