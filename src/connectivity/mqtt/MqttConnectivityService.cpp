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

#include "connectivity/mqtt/MqttConnectivityService.h"
#include "model/Message.h"

namespace wolkabout
{
MqttConnectivityService::MqttConnectivityService(std::shared_ptr<MqttClient> mqttClient, std::string key,
                                                 std::string password, std::string host)
: m_mqttClient(std::move(mqttClient))
, m_key(std::move(key))
, m_password(std::move(password))
, m_host(std::move(host))
, m_lastWillChannel("")
, m_lastWillPayload("")
, m_lastWillRetain(false)
, m_connected(false)
{
    m_mqttClient->onMessageReceived([this](std::string topic, std::string message) -> void {
        if (auto handler = m_listener.lock())
        {
            handler->messageReceived(topic, message);
        }
    });

    m_mqttClient->onConnectionLost([this]() -> void {
        if (auto handler = m_listener.lock())
        {
            handler->connectionLost();
        }
    });
}

bool MqttConnectivityService::connect()
{
    m_mqttClient->setLastWill(m_lastWillChannel, m_lastWillPayload, m_lastWillRetain);
    bool isConnected = m_mqttClient->connect(m_key, m_password, TRUST_STORE, m_host, m_key);
    if (isConnected)
    {
        if (auto handler = m_listener.lock())
        {
            const auto& topics = handler->getChannels();
            for (const std::string& topic : topics)
            {
                m_mqttClient->subscribe(topic);
            }
        }
    }

    return isConnected;
}

void MqttConnectivityService::disconnect()
{
    m_mqttClient->disconnect();
}

bool MqttConnectivityService::reconnect()
{
    disconnect();
    return connect();
}

bool MqttConnectivityService::isConnected()
{
    return m_mqttClient->isConnected();
}

bool MqttConnectivityService::publish(std::shared_ptr<Message> outboundMessage, bool persistent)
{
    return m_mqttClient->publish(outboundMessage->getChannel(), outboundMessage->getContent(), persistent);
}

void MqttConnectivityService::setUncontrolledDisonnectMessage(std::shared_ptr<Message> outboundMessage, bool persistent)
{
    m_lastWillChannel = outboundMessage->getChannel();
    m_lastWillPayload = outboundMessage->getContent();
    m_lastWillRetain = persistent;
}
}    // namespace wolkabout
