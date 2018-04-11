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

#ifndef REGISTRATIONRESPONSEHANDLER_H
#define REGISTRATIONRESPONSEHANDLER_H

#include "DeviceReregistrationResponse.h"
#include <string>

namespace wolkabout
{
class RegistrationResponseHandler
{
public:
    /**
     * @brief Registration Response Handler callback<br>
     *        Must be implemented as non blocking<br>
     *        Must be implemented as thread safe
     * @param key Device key
     * @param value Desired actuator value
     */
    virtual void operator()(const std::string& key, DeviceReregistrationResponse::Result result) = 0;

    virtual ~RegistrationResponseHandler() = default;
};
}    // namespace wolkabout

#endif    // REGISTRATIONRESPONSEHANDLER_H
