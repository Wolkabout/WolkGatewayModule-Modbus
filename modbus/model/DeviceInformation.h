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

/**
 * @brief Model class containing information necessary to create a single device for the module.
 *        Contained in the deviceConfiguration.json file.
 */
class DeviceInformation
{
public:
    DeviceInformation(const std::string& name, const std::string& key, DevicesConfigurationTemplate* deviceTemplate,
                      int slaveAddress);

    DeviceInformation(nlohmann::json j, DevicesConfigurationTemplate* templatePointer);

    DeviceInformation(const DeviceInformation& instance);

    explicit DeviceInformation(nlohmann::json j);

    const std::string& getName() const;

    const std::string& getKey() const;

    int getSlaveAddress() const;

    void setSlaveAddress(int slaveAddress);

    const std::string& getTemplateString() const;

    DevicesConfigurationTemplate* getTemplate() const;

    void setTemplate(DevicesConfigurationTemplate* templatePointer);

private:
    std::string m_name;
    std::string m_key;
    int m_slaveAddress = -1;
    std::string m_templateString;
    DevicesConfigurationTemplate* m_template;
};
}    // namespace wolkabout

#endif    // DEVICEINFORMATION_H
