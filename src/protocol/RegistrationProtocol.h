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

#ifndef REGISTRATIONPROTOCOL_H
#define REGISTRATIONPROTOCOL_H

#include "protocol/Protocol.h"
#include <memory>
#include <string>

namespace wolkabout
{
class Message;
class DeviceRegistrationRequest;
class DeviceRegistrationResponse;

class RegistrationProtocol : public Protocol
{
public:
    virtual std::shared_ptr<Message> makeMessage(const std::string& deviceKey,
                                                 const DeviceRegistrationRequest& request) const = 0;

    virtual std::shared_ptr<DeviceRegistrationResponse> makeRegistrationResponse(
      std::shared_ptr<Message> message) const = 0;

    inline Type getType() const override { return Protocol::Type::REGISTRATION; }
};
}    // namespace wolkabout

#endif
