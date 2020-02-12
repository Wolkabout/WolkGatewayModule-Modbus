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

#include <memory>

namespace wolkabout
{
class DeviceInformation
{
public:
    DeviceInformation() = default;

    DeviceInformation(std::string name, std::string key, std::weak_ptr<DeviceTemplate> deviceTemplate,
                      uint slaveAddress = -1);

    const std::string getName() const;

    const std::string getKey() const;

    uint getSlaveAddress() const;

    std::weak_ptr<DeviceTemplate> getTemplate();

private:
    std::string m_name;
    std::string m_key;
    uint m_slaveAddress;
    std::weak_ptr<DeviceTemplate> m_template;
};
}    // namespace wolkabout
