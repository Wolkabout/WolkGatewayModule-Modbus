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

#include "service/DataService.h"
#include "connectivity/ConnectivityService.h"
#include "model/ActuatorGetCommand.h"
#include "model/ActuatorSetCommand.h"
#include "model/ConfigurationSetCommand.h"
#include "model/Message.h"
#include "model/SensorReading.h"
#include "persistence/Persistence.h"
#include "protocol/DataProtocol.h"
#include "utilities/Logger.h"

#include <algorithm>
#include <cassert>
#include <functional>

namespace wolkabout
{
const std::string DataService::PERSISTENCE_KEY_DELIMITER = "+";

DataService::DataService(DataProtocol& protocol, Persistence& persistence, ConnectivityService& connectivityService,
                         const ActuatorSetHandler& actuatorSetHandler, const ActuatorGetHandler& actuatorGetHandler,
                         const ConfigurationSetHandler& configurationSetHandler,
                         const ConfigurationGetHandler& configurationGetHandler)
: m_protocol{protocol}
, m_persistence{persistence}
, m_connectivityService{connectivityService}
, m_actuatorSetHandler{actuatorSetHandler}
, m_actuatorGetHandler{actuatorGetHandler}
, m_configurationSetHandler{configurationSetHandler}
, m_configurationGetHandler{configurationGetHandler}
{
}

void DataService::messageReceived(std::shared_ptr<Message> message)
{
    assert(message);

    const std::string deviceKey = m_protocol.extractDeviceKeyFromChannel(message->getChannel());
    if (deviceKey.empty())
    {
        LOG(WARN) << "Unable to extract device key from channel: " << message->getChannel();
        return;
    }

    if (m_protocol.isActuatorGetMessage(*message))
    {
        auto command = m_protocol.makeActuatorGetCommand(*message);
        if (!command)
        {
            LOG(WARN) << "Unable to parse message contents: " << message->getContent();
            return;
        }

        if (m_actuatorGetHandler)
        {
            m_actuatorGetHandler(deviceKey, command->getReference());
        }
    }
    else if (m_protocol.isActuatorSetMessage(*message))
    {
        auto command = m_protocol.makeActuatorSetCommand(*message);
        if (!command)
        {
            LOG(WARN) << "Unable to parse message contents: " << message->getContent();
            return;
        }

        if (m_actuatorSetHandler)
        {
            m_actuatorSetHandler(deviceKey, command->getReference(), command->getValue());
        }
    }
    else if (m_protocol.isConfigurationGetMessage(*message))
    {
        if (m_configurationGetHandler)
        {
            m_configurationGetHandler(deviceKey);
        }
    }
    else if (m_protocol.isConfigurationSetMessage(*message))
    {
        auto command = m_protocol.makeConfigurationSetCommand(*message);
        if (!command)
        {
            LOG(WARN) << "Unable to parse message contents: " << message->getContent();
            return;
        }

        if (m_configurationSetHandler)
        {
            m_configurationSetHandler(deviceKey, command->getValues());
        }
    }
    else
    {
        LOG(WARN) << "Unable to parse message channel: " << message->getChannel();
    }
}

const Protocol& DataService::getProtocol()
{
    return m_protocol;
}

void DataService::addSensorReading(const std::string& deviceKey, const std::string& reference, const std::string& value,
                                   unsigned long long int rtc)
{
    auto sensorReading = std::make_shared<SensorReading>(value, reference, rtc);

    m_persistence.putSensorReading(makePersistenceKey(deviceKey, reference), sensorReading);
}

void DataService::addSensorReading(const std::string& deviceKey, const std::string& reference,
                                   const std::vector<std::string>& values, const std::string& delimiter,
                                   unsigned long long int rtc)
{
    auto sensorReading = std::make_shared<SensorReading>(values, reference, rtc);

    auto key = makePersistenceKey(deviceKey, reference);

    m_sensorDelimiters[key] = delimiter;

    m_persistence.putSensorReading(key, sensorReading);
}

void DataService::addAlarm(const std::string& deviceKey, const std::string& reference, bool active,
                           unsigned long long int rtc)
{
    auto alarm = std::make_shared<Alarm>(active, reference, rtc);

    m_persistence.putAlarm(makePersistenceKey(deviceKey, reference), alarm);
}

void DataService::addActuatorStatus(const std::string& deviceKey, const std::string& reference,
                                    const std::string& value, ActuatorStatus::State state)
{
    auto actuatorStatusWithRef = std::make_shared<ActuatorStatus>(value, reference, state);

    m_persistence.putActuatorStatus(makePersistenceKey(deviceKey, reference), actuatorStatusWithRef);
}

void DataService::addConfiguration(const std::string& deviceKey, const std::vector<ConfigurationItem>& configuration,
                                   const std::map<std::string, std::string>& delimiters)
{
    auto conf = std::make_shared<std::vector<ConfigurationItem>>(configuration);

    m_configurationDelimiters[deviceKey] = delimiters;

    m_persistence.putConfiguration(deviceKey, conf);
}

void DataService::publishSensorReadings()
{
    for (const auto& key : m_persistence.getSensorReadingsKeys())
    {
        publishSensorReadingsForPersistanceKey(key);
    }
}

void DataService::publishSensorReadings(const std::string& deviceKey)
{
    const auto& readingskeys = m_persistence.getSensorReadingsKeys();

    const std::vector<std::string> matchingReadingsKeys = findMatchingPersistanceKeys(deviceKey, readingskeys);

    for (const std::string& matchingKey : matchingReadingsKeys)
    {
        publishSensorReadingsForPersistanceKey(matchingKey);
    }
}

void DataService::publishSensorReadingsForPersistanceKey(const std::string& persistanceKey)
{
    const auto sensorReadings = m_persistence.getSensorReadings(persistanceKey, PUBLISH_BATCH_ITEMS_COUNT);

    if (sensorReadings.empty())
    {
        return;
    }

    auto pair = parsePersistenceKey(persistanceKey);
    if (pair.first.empty() || pair.second.empty())
    {
        LOG(ERROR) << "Unable to parse persistence key: " << persistanceKey;
        m_persistence.removeSensorReadings(persistanceKey, PUBLISH_BATCH_ITEMS_COUNT);
        return;
    }

    const auto delimiter = getSensorDelimiter(persistanceKey);

    const std::shared_ptr<Message> outboundMessage = m_protocol.makeMessage(pair.first, sensorReadings, delimiter);

    if (!outboundMessage)
    {
        LOG(ERROR) << "Unable to create message from readings: " << persistanceKey;
        m_persistence.removeSensorReadings(persistanceKey, PUBLISH_BATCH_ITEMS_COUNT);
        return;
    }

    if (m_connectivityService.publish(outboundMessage))
    {
        m_persistence.removeSensorReadings(persistanceKey, PUBLISH_BATCH_ITEMS_COUNT);

        // proceed to publish next batch only if publish is successfull
        publishSensorReadingsForPersistanceKey(persistanceKey);
    }
}

void DataService::publishAlarms()
{
    for (const auto& key : m_persistence.getAlarmsKeys())
    {
        publishAlarmsForPersistanceKey(key);
    }
}

void DataService::publishAlarms(const std::string& deviceKey)
{
    const auto& alarmsKeys = m_persistence.getAlarmsKeys();

    const std::vector<std::string> matchingAlarmsKeys = findMatchingPersistanceKeys(deviceKey, alarmsKeys);

    for (const std::string& matchingKey : matchingAlarmsKeys)
    {
        publishAlarmsForPersistanceKey(matchingKey);
    }
}

void DataService::publishAlarmsForPersistanceKey(const std::string& persistanceKey)
{
    const auto alarms = m_persistence.getAlarms(persistanceKey, PUBLISH_BATCH_ITEMS_COUNT);

    if (alarms.empty())
    {
        return;
    }

    auto pair = parsePersistenceKey(persistanceKey);
    if (pair.first.empty() || pair.second.empty())
    {
        LOG(ERROR) << "Unable to parse persistence key: " << persistanceKey;
        m_persistence.removeAlarms(persistanceKey, PUBLISH_BATCH_ITEMS_COUNT);
        return;
    }

    const std::shared_ptr<Message> outboundMessage = m_protocol.makeMessage(pair.first, alarms);

    if (!outboundMessage)
    {
        LOG(ERROR) << "Unable to create message from alarms: " << persistanceKey;
        m_persistence.removeAlarms(persistanceKey, PUBLISH_BATCH_ITEMS_COUNT);
        return;
    }

    if (m_connectivityService.publish(outboundMessage))
    {
        m_persistence.removeAlarms(persistanceKey, PUBLISH_BATCH_ITEMS_COUNT);

        // proceed to publish next batch only if publish is successfull
        publishAlarmsForPersistanceKey(persistanceKey);
    }
}

void DataService::publishActuatorStatuses()
{
    for (const auto& key : m_persistence.getActuatorStatusesKeys())
    {
        publishActuatorStatusesForPersistanceKey(key);
    }
}
void DataService::publishActuatorStatuses(const std::string& deviceKey)
{
    const auto& actuatorStatusesKeys = m_persistence.getActuatorStatusesKeys();

    const std::vector<std::string> matchingActuatorStatusesKeys =
      findMatchingPersistanceKeys(deviceKey, actuatorStatusesKeys);

    for (const std::string& matchingKey : matchingActuatorStatusesKeys)
    {
        publishActuatorStatusesForPersistanceKey(matchingKey);
    }
}

void DataService::publishActuatorStatusesForPersistanceKey(const std::string& persistanceKey)
{
    const auto actuatorStatus = m_persistence.getActuatorStatus(persistanceKey);

    if (!actuatorStatus)
    {
        return;
    }

    auto pair = parsePersistenceKey(persistanceKey);
    if (pair.first.empty() || pair.second.empty())
    {
        LOG(ERROR) << "Unable to parse persistence key: " << persistanceKey;
        m_persistence.removeActuatorStatus(persistanceKey);
        return;
    }

    const std::shared_ptr<Message> outboundMessage = m_protocol.makeMessage(pair.first, {actuatorStatus});

    if (!outboundMessage)
    {
        LOG(ERROR) << "Unable to create message from actuator status: " << persistanceKey;
        m_persistence.removeActuatorStatus(persistanceKey);
        return;
    }

    if (m_connectivityService.publish(outboundMessage))
    {
        m_persistence.removeActuatorStatus(persistanceKey);
    }
}

void DataService::publishConfiguration()
{
    for (const auto& key : m_persistence.getConfigurationKeys())
    {
        publishConfigurationForPersistanceKey(key);
    }
}

void DataService::publishConfiguration(const std::string& deviceKey)
{
    publishConfigurationForPersistanceKey(deviceKey);
}

void DataService::publishConfigurationForPersistanceKey(const std::string& persistanceKey)
{
    const auto configuration = m_persistence.getConfiguration(persistanceKey);

    if (!configuration)
    {
        return;
    }

    const auto delimiters = getConfigurationDelimiters(persistanceKey);

    const std::shared_ptr<Message> outboundMessage = m_protocol.makeMessage(persistanceKey, *configuration, delimiters);

    if (!outboundMessage)
    {
        LOG(ERROR) << "Unable to create message from configuration: " << persistanceKey;
        m_persistence.removeConfiguration(persistanceKey);
        return;
    }

    if (m_connectivityService.publish(outboundMessage))
    {
        m_persistence.removeConfiguration(persistanceKey);
    }
}

std::string DataService::makePersistenceKey(const std::string& deviceKey, const std::string& reference) const
{
    return deviceKey + PERSISTENCE_KEY_DELIMITER + reference;
}

std::pair<std::string, std::string> DataService::parsePersistenceKey(const std::string& key) const
{
    auto pos = key.find(PERSISTENCE_KEY_DELIMITER);
    if (pos == std::string::npos)
    {
        return std::make_pair("", "");
    }

    auto deviceKey = key.substr(0, pos);
    auto reference = key.substr(pos + PERSISTENCE_KEY_DELIMITER.size(), std::string::npos);

    return std::make_pair(deviceKey, reference);
}

std::string DataService::getSensorDelimiter(const std::string& key) const
{
    const auto it = m_sensorDelimiters.find(key);

    return it != m_sensorDelimiters.end() ? it->second : "";
}

std::map<std::string, std::string> DataService::getConfigurationDelimiters(const std::string& key) const
{
    const auto it = m_configurationDelimiters.find(key);

    return it != m_configurationDelimiters.end() ? it->second : std::map<std::string, std::string>{};
}

std::vector<std::string> DataService::findMatchingPersistanceKeys(const std::string& deviceKey,
                                                                  const std::vector<std::string>& persistanceKeys) const
{
    std::vector<std::string> matchingKeys;

    for (const auto& key : persistanceKeys)
    {
        auto pair = parsePersistenceKey(key);
        if (pair.first == deviceKey)
        {
            matchingKeys.push_back(key);
        }
    }

    return matchingKeys;
}
}    // namespace wolkabout
