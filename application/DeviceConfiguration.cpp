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

#include "DeviceConfiguration.h"
#include "utilities/FileSystemUtils.h"
#include "utilities/json.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace wolkabout
{
using nlohmann::json;

DeviceConfiguration::DeviceConfiguration(std::string name, std::string key, std::string protocol,
                                         std::string localMqttUri)
: m_name(std::move(name))
, m_key(std::move(key))
, m_protocol(std::move(protocol))
, m_localMqttUri(std::move(localMqttUri))
{
}

const std::string& DeviceConfiguration::getName() const
{
    return m_name;
}

const std::string& DeviceConfiguration::getKey() const
{
    return m_key;
}

const std::string& DeviceConfiguration::getProtocol() const
{
    return m_protocol;
}

const std::string& DeviceConfiguration::getLocalMqttUri() const
{
    return m_localMqttUri;
}

wolkabout::DeviceConfiguration DeviceConfiguration::fromJsonFile(const std::string& deviceConfigurationFile)
{
    if (!FileSystemUtils::isFilePresent(deviceConfigurationFile))
    {
        throw std::logic_error("Given device configuration file does not exist.");
    }

    std::string deviceConfigurationJson;
    if (!FileSystemUtils::readFileContent(deviceConfigurationFile, deviceConfigurationJson))
    {
        throw std::logic_error("Unable to read device configuration file.");
    }

    auto j = json::parse(deviceConfigurationJson);
    const auto name = j.at("name").get<std::string>();
    const auto key = j.at("key").get<std::string>();
    const auto protocol = j.at("protocol").get<std::string>();
    const auto localMqttUri = j.at("localMqttUri").get<std::string>();

    return DeviceConfiguration(name, key, protocol, localMqttUri);
}
}    // namespace wolkabout
