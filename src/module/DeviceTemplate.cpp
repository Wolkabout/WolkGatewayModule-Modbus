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

#include "DeviceTemplate.h"

namespace wolkabout
{
using nlohmann::json;

DeviceTemplate::DeviceTemplate(std::string name, std::vector<ModbusRegisterMapping> mappings)
: m_name(name), m_mappings(mappings)
{
}

DeviceTemplate::DeviceTemplate(nlohmann::json j)
{
    try
    {
        m_name = j.at("name").get<std::string>();
    }
    catch (std::exception&)
    {
        throw std::logic_error("Missing device templated field : name");
    }

    for (auto const& mapping : j["mappings"].get<json::array_t>)
    {
        m_mappings.emplace_back(ModbusRegisterMapping(mapping));
    }
}
}    // namespace wolkabout
