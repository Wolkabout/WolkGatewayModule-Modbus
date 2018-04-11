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

#ifndef JSONPROTOCOL_H
#define JSONPROTOCOL_H

#include "protocol/DataProtocol.h"

#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class JsonProtocol : public DataProtocol
{
public:
    const std::string& getName() const override;

    const std::vector<std::string>& getInboundChannels() const override;

    bool isActuatorSetMessage(const std::string& channel) const override;
    bool isActuatorGetMessage(const std::string& channel) const override;

    std::unique_ptr<ActuatorGetCommand> makeActuatorGetCommand(std::shared_ptr<Message> message) const override;
    std::unique_ptr<ActuatorSetCommand> makeActuatorSetCommand(std::shared_ptr<Message> message) const override;

    std::shared_ptr<Message> makeMessage(const std::string& deviceKey,
                                         std::vector<std::shared_ptr<SensorReading>> sensorReadings) const override;
    std::shared_ptr<Message> makeMessage(const std::string& deviceKey,
                                         std::vector<std::shared_ptr<Alarm>> alarms) const override;
    std::shared_ptr<Message> makeMessage(const std::string& deviceKey,
                                         std::vector<std::shared_ptr<ActuatorStatus>> actuatorStatuses) const override;

    std::string extractReferenceFromChannel(const std::string& topic) const override;
    std::string extractDeviceKeyFromChannel(const std::string& topic) const override;

private:
    static const std::string NAME;

    static const std::string CHANNEL_DELIMITER;
    static const std::string CHANNEL_WILDCARD;

    static const std::string DEVICE_TYPE;
    static const std::string REFERENCE_TYPE;
    static const std::string DEVICE_TO_PLATFORM_TYPE;
    static const std::string PLATFORM_TO_DEVICE_TYPE;

    static const std::string DEVICE_PATH_PREFIX;
    static const std::string REFERENCE_PATH_PREFIX;
    static const std::string DEVICE_TO_PLATFORM_DIRECTION;
    static const std::string PLATFORM_TO_DEVICE_DIRECTION;

    static const std::string SENSOR_READING_TOPIC_ROOT;
    static const std::string EVENTS_TOPIC_ROOT;
    static const std::string ACTUATION_STATUS_TOPIC_ROOT;
    static const std::string CONFIGURATION_SET_RESPONSE_TOPIC_ROOT;
    static const std::string CONFIGURATION_GET_RESPONSE_TOPIC_ROOT;

    static const std::string ACTUATION_SET_TOPIC_ROOT;
    static const std::string ACTUATION_GET_TOPIC_ROOT;
    static const std::string CONFIGURATION_SET_REQUEST_TOPIC_ROOT;
    static const std::string CONFIGURATION_GET_REQUEST_TOPIC_ROOT;

    static const std::vector<std::string> INBOUND_CHANNELS;
};
}    // namespace wolkabout

#endif
