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

#ifndef JSONREGISTRATIONPROTOCOL_H
#define JSONREGISTRATIONPROTOCOL_H

#include "protocol/RegistrationProtocol.h"

#include <vector>

namespace wolkabout
{
class JsonRegistrationProtocol : public RegistrationProtocol
{
public:
    const std::string& getName() const override;
    const std::vector<std::string>& getInboundChannels() const override;
    std::string extractDeviceKeyFromChannel(const std::string& topic) const override;

    std::shared_ptr<Message> makeMessage(const std::string& deviceKey,
                                         const DeviceRegistrationRequest& request) const override;
    std::shared_ptr<DeviceRegistrationResponse> makeRegistrationResponse(
      std::shared_ptr<Message> message) const override;

private:
    static const std::string NAME;

    static const std::string CHANNEL_DELIMITER;
    static const std::string CHANNEL_WILDCARD;
    static const std::string DEVICE_PATH_PREFIX;
    static const std::string DEVICE_TO_PLATFORM_DIRECTION;
    static const std::string PLATFORM_TO_DEVICE_DIRECTION;

    static const std::string DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT;
    static const std::string DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT;
    static const std::string DEVICE_REREGISTRATION_RESPONSE_TOPIC_ROOT;
    static const std::string DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT;

    static const std::vector<std::string> INBOUND_CHANNELS;

    static const std::string REGISTRATION_RESPONSE_OK;
    static const std::string REGISTRATION_RESPONSE_ERROR_KEY_CONFLICT;
    static const std::string REGISTRATION_RESPONSE_ERROR_MANIFEST_CONFLICT;
    static const std::string REGISTRATION_RESPONSE_ERROR_MAX_NUMBER_OF_DEVICES_EXCEEDED;
    static const std::string REGISTRATION_RESPONSE_ERROR_READING_PAYLOAD;
    static const std::string REGISTRATION_RESPONSE_ERROR_GATEWAY_NOT_FOUND;
    static const std::string REGISTRATION_RESPONSE_ERROR_NO_GATEWAY_MANIFEST;
};
}    // namespace wolkabout

#endif    // JSONREGISTRATIONPROTOCOL_H
