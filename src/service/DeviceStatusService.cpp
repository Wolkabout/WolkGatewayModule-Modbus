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

#include "service/DeviceStatusService.h"
#include "connectivity/ConnectivityService.h"
#include "model/DeviceStatusResponse.h"
#include "model/Message.h"
#include "protocol/StatusProtocol.h"

#include "utilities/Logger.h"

namespace wolkabout
{
DeviceStatusService::DeviceStatusService(StatusProtocol& protocol, ConnectivityService& connectivityService,
                                         const StatusRequestHandler& statusRequestHandler)
: m_protocol{protocol}, m_connectivityService{connectivityService}, m_statusRequestHandler{statusRequestHandler}
{
}

void DeviceStatusService::messageReceived(std::shared_ptr<Message> message)
{
    const std::string deviceKey = m_protocol.extractDeviceKeyFromChannel(message->getChannel());
    if (deviceKey.empty())
    {
        LOG(WARN) << "Unable to extract device key from channel: " << message->getChannel();
        return;
    }

    if (m_protocol.isStatusRequestMessage(message->getChannel()))
    {
        m_statusRequestHandler(deviceKey);
    }
    else
    {
        LOG(WARN) << "Unable to parse message channel: " << message->getChannel();
    }
}

const Protocol& DeviceStatusService::getProtocol()
{
    return m_protocol;
}

void DeviceStatusService::publishDeviceStatus(const std::string& deviceKey, DeviceStatus status)
{
    auto statusResponse = std::make_shared<DeviceStatusResponse>(status);

    const std::shared_ptr<Message> outboundMessage = m_protocol.makeMessage(deviceKey, statusResponse);

    if (!outboundMessage || !m_connectivityService.publish(outboundMessage))
    {
        LOG(INFO) << "Status not published for device: " << deviceKey;
    }
}

void DeviceStatusService::devicesUpdated(const std::vector<std::string>& deviceKeys)
{
    auto lastWillMessage = m_protocol.makeLastWillMessage(deviceKeys);

    if (!lastWillMessage)
    {
        LOG(WARN) << "Unable to make lastwill message";
        return;
    }

    m_connectivityService.setUncontrolledDisonnectMessage(lastWillMessage);
}
}    // namespace wolkabout
