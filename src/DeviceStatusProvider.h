/*
 * Copyright 2018 WolkAbout Technology s.r.o.
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
#ifndef DEVICESTATUSPROVIDER_H
#define DEVICESTATUSPROVIDER_H

#include "model/DeviceStatus.h"

#include <string>

namespace wolkabout
{
class DeviceStatusProvider
{
public:
    virtual ~DeviceStatusProvider() = default;

    DeviceStatus operator()(const std::string& deviceKey) { return getStatus(deviceKey); }

private:
    /**
     * @brief Device status provider callback<br>
     *        Must be implemented as non blocking<br>
     *        Must be implemented as thread safe
     * @param deviceKey Key of the device which has the acuator
     * @param reference Actuator reference
     * @return DeviceStatus of requested actuator
     */
    virtual DeviceStatus getStatus(const std::string& deviceKey) = 0;
};
}    // namespace wolkabout

#endif    // DEVICESTATUSPROVIDER_H
