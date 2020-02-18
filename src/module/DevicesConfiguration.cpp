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

#include "DevicesConfiguration.h"
#include "utilities/json.hpp"

#include <string>

namespace wolkabout
{
using nlohmann::json;

DevicesConfiguration::DevicesConfiguration(std::map<std::string, std::unique_ptr<DeviceTemplateModule>>& templates,
                                           std::map<std::string, std::unique_ptr<DeviceInformation>>& devices)
: m_templates(templates), m_devices(devices)
{
}

DevicesConfiguration::DevicesConfiguration(nlohmann::json j) : m_templates(), m_devices()
{
    for (auto const& deviceTemplate : j["templates"].get<json::array_t>())
    {
        std::unique_ptr<DeviceTemplateModule> templatePointer(new DeviceTemplateModule(deviceTemplate));
        m_templates.emplace(templatePointer->getName(), std::move(templatePointer));
    }

    for (json::object_t deviceInformation : j["devices"].get<json::array_t>())
    {
        std::unique_ptr<DeviceInformation> deviceInfo(new DeviceInformation(deviceInformation));
        if (m_templates.find(deviceInfo->getTemplateString()) != m_templates.end())
        {
            deviceInfo->setTemplate(&(m_templates.at(deviceInfo->getTemplateString())));
        }
        else
        {
            throw std::logic_error("Invalid template found for device " + deviceInfo->getName() + " (" +
                                   deviceInfo->getTemplateString() + ").");
        }
    }
}

std::map<std::string, std::unique_ptr<DeviceTemplateModule>>& DevicesConfiguration::getTemplates()
{
    return m_templates;
}

std::map<std::string, std::unique_ptr<DeviceInformation>>& DevicesConfiguration::getInformation()
{
    return m_devices;
}
}    // namespace wolkabout
