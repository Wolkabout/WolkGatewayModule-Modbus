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

#ifndef WOLKGATEWAYMODBUSMODULE_DEVICETEMPLATEMODULE_H
#define WOLKGATEWAYMODBUSMODULE_DEVICETEMPLATEMODULE_H

#include "modbus/ModbusRegisterMapping.h"
#include "utilities/json.hpp"

#include <string>

namespace wolkabout
{
using nlohmann::json;

class DeviceTemplateModule
{
public:
    DeviceTemplateModule() = default;

    DeviceTemplateModule(std::string& name, std::vector<ModbusRegisterMapping>& mappings);

    explicit DeviceTemplateModule(nlohmann::json json);

    const std::string& getName() const;

    const std::vector<ModbusRegisterMapping>& getMappings() const;

private:
    std::string m_name;
    std::vector<ModbusRegisterMapping> m_mappings;
};
}    // namespace wolkabout

#endif    // WOLKGATEWAYMODBUSMODULE_DEVICETEMPLATEMODULE_H