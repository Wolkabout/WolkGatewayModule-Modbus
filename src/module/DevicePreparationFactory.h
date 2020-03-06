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

#ifndef WOLKGATEWAYMODBUSMODULE_DEVICEPREPARATIONFACTORY_H
#define WOLKGATEWAYMODBUSMODULE_DEVICEPREPARATIONFACTORY_H

#include "DeviceInformation.h"
#include "DevicesConfigurationTemplate.h"
#include "ModuleConfiguration.h"
#include "model/Device.h"
#include "model/DeviceTemplate.h"

#include <map>
#include <memory>
#include <string>

namespace wolkabout
{
class DevicePreparationFactory
{
public:
    DevicePreparationFactory(const std::map<std::string, std::unique_ptr<DevicesConfigurationTemplate>>& inputTemplates,
                             const std::map<std::string, std::unique_ptr<DeviceInformation>>& inputDevices,
                             ModuleConfiguration::ConnectionType connectionType);

    const std::map<std::string, std::unique_ptr<wolkabout::DeviceTemplate>>& getTemplates() const;

    const std::map<int, std::unique_ptr<wolkabout::Device>>& getDevices() const;

    const std::map<std::string, std::vector<int>>& getTemplateDeviceMap() const;

private:
    std::map<std::string, std::unique_ptr<DevicesConfigurationTemplate>> m_inputTemplates;
    std::map<std::string, std::unique_ptr<DeviceInformation>> m_inputDevices;

    ModuleConfiguration::ConnectionType m_connectionType;

    std::map<std::string, std::unique_ptr<wolkabout::DeviceTemplate>> m_templates;
    std::map<int, std::unique_ptr<wolkabout::Device>> m_devices;
    std::map<std::string, std::vector<int>> m_usedTemplates;
};
}    // namespace wolkabout

#endif    // WOLKGATEWAYMODBUSMODULE_DEVICEPREPARATIONFACTORY_H
