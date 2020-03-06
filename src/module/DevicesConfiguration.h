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

#ifndef WOLKGATEWAYMODBUSMODULE_DEVICESCONFIGURATION_H
#define WOLKGATEWAYMODBUSMODULE_DEVICESCONFIGURATION_H

#include "DeviceInformation.h"

namespace wolkabout
{
using nlohmann::json;

class DevicesConfiguration
{
public:
    explicit DevicesConfiguration(nlohmann::json j);

    const std::map<std::string, std::unique_ptr<DevicesConfigurationTemplate>>& getTemplates();

    const std::map<std::string, std::unique_ptr<DeviceInformation>>& getDevices();

private:
    std::map<std::string, std::unique_ptr<DevicesConfigurationTemplate>> m_templates;
    std::map<std::string, std::unique_ptr<DeviceInformation>> m_devices;
};
}    // namespace wolkabout

#endif    // WOLKGATEWAYMODBUSMODULE_DEVICESCONFIGURATION_H
