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

#include "protocol/json/JsonStatusProtocol.h"
#include "model/DeviceStatus.h"
#include "model/DeviceStatusResponse.h"
#include "model/Message.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"
#include "utilities/json.hpp"

using nlohmann::json;

namespace wolkabout
{
const std::string JsonStatusProtocol::NAME = "JsonStatusProtocol";

const std::string JsonStatusProtocol::CHANNEL_DELIMITER = "/";
const std::string JsonStatusProtocol::CHANNEL_WILDCARD = "#";
const std::string JsonStatusProtocol::DEVICE_PATH_PREFIX = "d/";

const std::string JsonStatusProtocol::LAST_WILL_TOPIC = "lastwill";
const std::string JsonStatusProtocol::DEVICE_STATUS_REQUEST_TOPIC_ROOT = "p2d/status/";
const std::string JsonStatusProtocol::DEVICE_STATUS_RESPONSE_TOPIC_ROOT = "d2p/status/";

const std::vector<std::string> JsonStatusProtocol::INBOUND_CHANNELS = {DEVICE_STATUS_REQUEST_TOPIC_ROOT +
                                                                       CHANNEL_WILDCARD};

const std::string JsonStatusProtocol::STATUS_RESPONSE_STATE_FIELD = "state";
const std::string JsonStatusProtocol::STATUS_RESPONSE_STATUS_CONNECTED = "CONNECTED";
const std::string JsonStatusProtocol::STATUS_RESPONSE_STATUS_SLEEP = "SLEEP";
const std::string JsonStatusProtocol::STATUS_RESPONSE_STATUS_SERVICE = "SERVICE";
const std::string JsonStatusProtocol::STATUS_RESPONSE_STATUS_OFFLINE = "OFFLINE";

void to_json(json& j, const DeviceStatusResponse& p)
{
    const std::string status = [&]() -> std::string {
        switch (p.getStatus())
        {
        case DeviceStatus::CONNECTED:
        {
            return JsonStatusProtocol::STATUS_RESPONSE_STATUS_CONNECTED;
        }
        case DeviceStatus::SLEEP:
        {
            return JsonStatusProtocol::STATUS_RESPONSE_STATUS_SLEEP;
        }
        case DeviceStatus::SERVICE:
        {
            return JsonStatusProtocol::STATUS_RESPONSE_STATUS_SERVICE;
        }
        case DeviceStatus::OFFLINE:
        default:
        {
            return JsonStatusProtocol::STATUS_RESPONSE_STATUS_OFFLINE;
        }
        }
    }();

    j = json{{JsonStatusProtocol::STATUS_RESPONSE_STATE_FIELD, status}};
}

void to_json(json& j, const std::shared_ptr<DeviceStatusResponse>& p)
{
    if (!p)
    {
        return;
    }

    to_json(j, *p);
}

const std::string& JsonStatusProtocol::getName() const
{
    return NAME;
}

const std::vector<std::string>& JsonStatusProtocol::getInboundChannels() const
{
    LOG(TRACE) << METHOD_INFO;
    return INBOUND_CHANNELS;
}

bool JsonStatusProtocol::isStatusRequestMessage(const std::string& topic) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(topic, DEVICE_STATUS_REQUEST_TOPIC_ROOT);
}

std::shared_ptr<Message> JsonStatusProtocol::makeMessage(const std::string& deviceKey,
                                                         std::shared_ptr<DeviceStatusResponse> response) const
{
    LOG(TRACE) << METHOD_INFO;

    const json jPayload(response);
    const std::string topic = DEVICE_STATUS_RESPONSE_TOPIC_ROOT + DEVICE_PATH_PREFIX + deviceKey;

    const std::string payload = jPayload.dump();

    return std::make_shared<Message>(payload, topic);
}

std::shared_ptr<Message> JsonStatusProtocol::makeLastWillMessage(const std::vector<std::string>& deviceKeys) const
{
    LOG(TRACE) << METHOD_INFO;

    if (deviceKeys.size() == 1)
    {
        const std::string topic = LAST_WILL_TOPIC + CHANNEL_DELIMITER + deviceKeys.front();
        const std::string payload = "";

        return std::make_shared<Message>(payload, topic);
    }
    else
    {
        const std::string topic = LAST_WILL_TOPIC;
        const json jPayload(deviceKeys);
        const std::string payload = jPayload.dump();

        return std::make_shared<Message>(payload, topic);
    }
}

std::string JsonStatusProtocol::extractDeviceKeyFromChannel(const std::string& topic) const
{
    LOG(TRACE) << METHOD_INFO;

    const std::string devicePathPrefix = CHANNEL_DELIMITER + DEVICE_PATH_PREFIX;

    const auto deviceKeyStartPosition = topic.find(devicePathPrefix);
    if (deviceKeyStartPosition != std::string::npos)
    {
        const auto keyEndPosition = topic.find(CHANNEL_DELIMITER, deviceKeyStartPosition + devicePathPrefix.size());

        const auto pos = deviceKeyStartPosition + devicePathPrefix.size();

        return topic.substr(pos, keyEndPosition - pos);
    }

    return "";
}
}    // namespace wolkabout
