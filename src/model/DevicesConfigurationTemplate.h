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

#include "model/ModbusRegisterMapping.h"

namespace wolkabout
{
using nlohmann::json;

class DevicesConfigurationTemplate
{
public:
    DevicesConfigurationTemplate(const std::string& name, const std::vector<ModbusRegisterMapping>& mappings);

    DevicesConfigurationTemplate(const DevicesConfigurationTemplate& instance);

    explicit DevicesConfigurationTemplate(nlohmann::json json);

    const std::string& getName() const;

    const std::vector<ModbusRegisterMapping>& getMappings() const;

private:
    std::string m_name;
    std::vector<ModbusRegisterMapping> m_mappings;
};
}    // namespace wolkabout

#endif    // DEVICETEMPLATEMODULE_H
