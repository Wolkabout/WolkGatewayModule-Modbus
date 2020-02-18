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

#ifndef WOLKGATEWAYMODBUSMODULE_DEVICEINFORMATION_H
#define WOLKGATEWAYMODBUSMODULE_DEVICEINFORMATION_H

#include "DeviceTemplateModule.h"
#include "utilities/json.hpp"

#include <memory>

namespace wolkabout
{
using nlohmann::json;

class DeviceInformation
{
public:
    DeviceInformation() = default;

    DeviceInformation(std::string& name, std::string& key, std::unique_ptr<DeviceTemplateModule>* deviceTemplate,
                      int slaveAddress);

    explicit DeviceInformation(nlohmann::json j);

    const std::string& getName() const;

    const std::string& getKey() const;

    int getSlaveAddress() const;

    std::unique_ptr<DeviceTemplateModule>* getTemplate();

    const std::string& getTemplateString() const;

    void setTemplate(std::unique_ptr<DeviceTemplateModule>* templatePointer);

private:
    std::string m_name;
    std::string m_key;
    int m_slaveAddress = -1;
    std::string m_templateString;
    std::unique_ptr<DeviceTemplateModule>* m_template{};
};
}    // namespace wolkabout

#endif    // WOLKGATEWAYMODBUSMODULE_DEVICEINFORMATION_H
