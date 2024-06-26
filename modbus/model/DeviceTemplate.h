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

#ifndef DEVICETEMPLATEMODULE_H
#define DEVICETEMPLATEMODULE_H

#include "modbus/model/ModuleMapping.h"

namespace wolkabout
{
namespace modbus
{
using nlohmann::json;

/**
 * @brief Model class containing information for a single template created in a
 *        deviceConfiguration.json file.
 */
class DeviceTemplate
{
public:
    DeviceTemplate(std::string name, std::vector<ModuleMapping> mappings);

    DeviceTemplate(const DeviceTemplate& instance);

    explicit DeviceTemplate(nlohmann::json json);

    const std::string& getName() const;

    const std::vector<ModuleMapping>& getMappings() const;

private:
    std::string m_name;
    std::vector<ModuleMapping> m_mappings;
};
}    // namespace modbus
}    // namespace wolkabout

#endif    // DEVICETEMPLATEMODULE_H
