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

#include "modbus/model/DeviceTemplate.h"

using nlohmann::json;

namespace wolkabout
{
namespace modbus
{
/**
 * @brief Model class containing information necessary to create a single device for the module.
 *        Contained in the deviceConfiguration.json file.
 */
class DeviceInformation
{
public:
    DeviceInformation(std::string name, std::string key, const DeviceTemplate& deviceTemplate,
                      std::uint16_t slaveAddress);

    DeviceInformation(nlohmann::json j, const DeviceTemplate& deviceTemplate);

    DeviceInformation(const DeviceInformation& instance) = default;

    const std::string& getName() const;

    const std::string& getKey() const;

    std::uint16_t getSlaveAddress() const;

    void setSlaveAddress(std::uint16_t slaveAddress);

    const std::string& getTemplateString() const;

    const DeviceTemplate& getTemplate() const;

private:
    std::string m_name;
    std::string m_key;
    std::uint16_t m_slaveAddress;
    std::string m_templateString;
    const DeviceTemplate& m_template;
};
}    // namespace modbus
}    // namespace wolkabout

#endif    // DEVICEINFORMATION_H
