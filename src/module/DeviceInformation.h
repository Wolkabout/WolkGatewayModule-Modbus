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

#ifndef DEVICEINFORMATION_H
#define DEVICEINFORMATION_H

#include "DevicesConfigurationTemplate.h"

namespace wolkabout
{
using nlohmann::json;

class DeviceInformation
{
public:
    DeviceInformation() = default;

    DeviceInformation(std::string& name, std::string& key,
                      std::unique_ptr<DevicesConfigurationTemplate>* deviceTemplate, int slaveAddress);

    explicit DeviceInformation(nlohmann::json j);

    DeviceInformation(nlohmann::json j, std::unique_ptr<DevicesConfigurationTemplate>* templatePointer);

    DeviceInformation(const DeviceInformation& instance);

    const std::string& getName() const;

    const std::string& getKey() const;

    int getSlaveAddress() const;

    const std::string& getTemplateString() const;

    std::unique_ptr<DevicesConfigurationTemplate>* getTemplate() const;

    void setTemplate(std::unique_ptr<DevicesConfigurationTemplate>* templatePointer);

private:
    std::string m_name;
    std::string m_key;
    int m_slaveAddress = -1;
    std::string m_templateString;
    std::unique_ptr<DevicesConfigurationTemplate>* m_template = nullptr;
};
}    // namespace wolkabout

#endif    // DEVICEINFORMATION_H
