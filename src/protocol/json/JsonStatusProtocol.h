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
#ifndef JSONSTATUSPROTOCOL_H
#define JSONSTATUSPROTOCOL_H

#include "protocol/StatusProtocol.h"

namespace wolkabout
{
class JsonStatusProtocol : public StatusProtocol
{
public:
    const std::string& getName() const override;

    const std::vector<std::string>& getInboundChannels() const override;

    std::string extractDeviceKeyFromChannel(const std::string& topic) const override;

    bool isStatusRequestMessage(const std::string& topic) const override;

    std::shared_ptr<Message> makeMessage(const std::string& deviceKey,
                                         std::shared_ptr<DeviceStatusResponse> response) const override;

    std::shared_ptr<Message> makeLastWillMessage(const std::vector<std::string>& deviceKeys) const override;

    static const std::string STATUS_RESPONSE_STATE_FIELD;
    static const std::string STATUS_RESPONSE_STATUS_CONNECTED;
    static const std::string STATUS_RESPONSE_STATUS_SLEEP;
    static const std::string STATUS_RESPONSE_STATUS_SERVICE;
    static const std::string STATUS_RESPONSE_STATUS_OFFLINE;

private:
    static const std::string NAME;

    static const std::string CHANNEL_DELIMITER;
    static const std::string CHANNEL_WILDCARD;
    static const std::string DEVICE_PATH_PREFIX;

    static const std::string LAST_WILL_TOPIC;
    static const std::string DEVICE_STATUS_REQUEST_TOPIC_ROOT;
    static const std::string DEVICE_STATUS_RESPONSE_TOPIC_ROOT;

    static const std::vector<std::string> INBOUND_CHANNELS;
};
}    // namespace wolkabout

#endif    // JSONSTATUSPROTOCOL_H
