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

#ifndef DATASERVICE_H
#define DATASERVICE_H

#include "InboundMessageHandler.h"
#include "model/ActuatorStatus.h"
#include <memory>

namespace wolkabout
{
class DataProtocol;
class Persistence;
class ConnectivityService;

typedef std::function<void(const std::string&, const std::string&, const std::string&)> ActuatorSetHandler;
typedef std::function<void(const std::string&, const std::string&)> ActuatorGetHandler;

class DataService : public MessageListener
{
public:
    DataService(DataProtocol& protocol, Persistence& persistence, ConnectivityService& connectivityService,
                const ActuatorSetHandler& actuatorSetHandler, const ActuatorGetHandler& actuatorGetHandler);

    void messageReceived(std::shared_ptr<Message> message) override;
    const Protocol& getProtocol() override;

    void addSensorReading(const std::string& deviceKey, const std::string& reference, const std::string& value,
                          unsigned long long int rtc);

    void addAlarm(const std::string& deviceKey, const std::string& reference, const std::string& value,
                  unsigned long long int rtc);

    void addActuatorStatus(const std::string& deviceKey, const std::string& reference, const std::string& value,
                           ActuatorStatus::State state);

    void publishSensorReadings();
    void publishSensorReadings(const std::string& deviceKey);

    void publishAlarms();
    void publishAlarms(const std::string& deviceKey);

    void publishActuatorStatuses();
    void publishActuatorStatuses(const std::string& deviceKey);

private:
    std::string makePersistenceKey(const std::string& deviceKey, const std::string& reference);
    std::pair<std::string, std::string> parsePersistenceKey(const std::string& key);

    DataProtocol& m_protocol;
    Persistence& m_persistence;
    ConnectivityService& m_connectivityService;

    ActuatorSetHandler m_actuatorSetHandler;
    ActuatorGetHandler m_actuatorGetHandler;

    static const std::string PERSISTENCE_KEY_DELIMITER;
    static const constexpr unsigned int PUBLISH_BATCH_ITEMS_COUNT = 50;
};
}    // namespace wolkabout

#endif
