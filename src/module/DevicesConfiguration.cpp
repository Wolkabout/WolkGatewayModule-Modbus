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
#include "utilities/FileSystemUtils.h"
#include "utilities/json.hpp"

#include <string>

namespace wolkabout
{
using nlohmann::json;

DevicesConfiguration::DevicesConfiguration(std::map<std::string, DeviceTemplate>& templates,
                                           std::map<std::string, DeviceInformation>& devices)
: m_templates(templates), m_devices(devices)
{
}

std::map<std::string, DeviceTemplate>& DevicesConfiguration::getTemplates()
{
    return m_templates;
}

std::map<std::string, DeviceInformation>& DevicesConfiguration::getInformation()
{
    return m_devices;
}

wolkabout::DevicesConfiguration DevicesConfiguration::fromJsonFile(const std::string& devicesConfigurationPath)
{
    if (!FileSystemUtils::isFilePresent(devicesConfigurationPath))
    {
        throw std::logic_error("Given devices configuration file does not exist.");
    }

    std::string devicesConfigurationJson;
    if (!FileSystemUtils::readFileContent(devicesConfigurationPath, devicesConfigurationJson))
    {
        throw std::logic_error("Unable to read module configuration file.");
    }

    auto j = json::parse(devicesConfigurationJson);

    std::map<std::string, DeviceTemplate> templates;
    for (auto const& deviceTemplate : j["templates"].get<json::array_t>)
    {
        DeviceTemplate dt(deviceTemplate);
        templates.emplace(dt.getName(), dt);
    }

    for (auto const& deviceInformation : j["devices"].get<json::array_t>)
    {
        std::string templateName = deviceInformation.at("template").get<std::string>();
    }
}
}    // namespace wolkabout
