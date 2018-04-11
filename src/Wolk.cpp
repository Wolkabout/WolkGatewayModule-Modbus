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

#include "Wolk.h"
#include "ActuationHandler.h"
#include "ActuatorStatusProvider.h"
#include "WolkBuilder.h"
#include "connectivity/ConnectivityService.h"
#include "model/ActuatorStatus.h"
#include "model/Device.h"
#include "service/DataService.h"
#include "service/DeviceRegistrationService.h"
#include "service/DeviceStatusService.h"
#include "service/FirmwareUpdateService.h"
#include "utilities/Logger.h"

#include <algorithm>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#define INSTANTIATE_ADD_SENSOR_READING_FOR(x)                                                                    \
    template void Wolk::addSensorReading<x>(const std::string& deviceKey, const std::string& reference, x value, \
                                            unsigned long long rtc)

namespace wolkabout
{
WolkBuilder Wolk::newBuilder()
{
    return WolkBuilder();
}

template <>
void Wolk::addSensorReading(const std::string& deviceKey, const std::string& reference, std::string value,
                            unsigned long long rtc)
{
    if (rtc == 0)
    {
        rtc = Wolk::currentRtc();
    }

    addToCommandBuffer([=]() -> void {
        if (!deviceExists(deviceKey))
        {
            LOG(ERROR) << "Device does not exist: " << deviceKey;
            return;
        }

        if (!sensorDefinedForDevice(deviceKey, reference))
        {
            LOG(ERROR) << "Sensor does not exist for device: " << deviceKey << ", " << reference;
            return;
        }

        m_dataService->addSensorReading(deviceKey, reference, value, rtc);
    });
}

template <typename T>
void Wolk::addSensorReading(const std::string& deviceKey, const std::string& reference, T value, unsigned long long rtc)
{
    addSensorReading(deviceKey, reference, std::to_string(value), rtc);
}

template <>
void Wolk::addSensorReading(const std::string& deviceKey, const std::string& reference, bool value,
                            unsigned long long rtc)
{
    addSensorReading(deviceKey, reference, std::string(value ? "true" : "false"), rtc);
}

template <>
void Wolk::addSensorReading(const std::string& deviceKey, const std::string& reference, char* value,
                            unsigned long long rtc)
{
    addSensorReading(deviceKey, reference, std::string(value), rtc);
}

template <>
void Wolk::addSensorReading(const std::string& deviceKey, const std::string& reference, const char* value,
                            unsigned long long rtc)
{
    addSensorReading(deviceKey, reference, std::string(value), rtc);
}

INSTANTIATE_ADD_SENSOR_READING_FOR(float);
INSTANTIATE_ADD_SENSOR_READING_FOR(double);
INSTANTIATE_ADD_SENSOR_READING_FOR(signed int);
INSTANTIATE_ADD_SENSOR_READING_FOR(signed long int);
INSTANTIATE_ADD_SENSOR_READING_FOR(signed long long int);
INSTANTIATE_ADD_SENSOR_READING_FOR(unsigned int);
INSTANTIATE_ADD_SENSOR_READING_FOR(unsigned long int);
INSTANTIATE_ADD_SENSOR_READING_FOR(unsigned long long int);

void Wolk::addAlarm(const std::string& deviceKey, const std::string& reference, const std::string& value,
                    unsigned long long rtc)
{
    if (rtc == 0)
    {
        rtc = Wolk::currentRtc();
    }

    addToCommandBuffer([=]() -> void {
        if (!deviceExists(deviceKey))
        {
            LOG(ERROR) << "Device does not exist: " << deviceKey;
            return;
        }

        if (!alarmDefinedForDevice(deviceKey, reference))
        {
            LOG(ERROR) << "Alarm does not exist for device: " << deviceKey << ", " << reference;
            return;
        }

        m_dataService->addAlarm(deviceKey, reference, value, rtc);
    });
}

void Wolk::publishActuatorStatus(const std::string& deviceKey, const std::string& reference)
{
    handleActuatorGetCommand(deviceKey, reference);
}

void Wolk::addDeviceStatus(const std::string& deviceKey, DeviceStatus status)
{
    addToCommandBuffer([=] {
        if (!deviceExists(deviceKey))
        {
            LOG(ERROR) << "Device does not exist: " << deviceKey;
            return;
        }

        m_deviceStatusService->publishDeviceStatus(deviceKey, status);
    });
}

void Wolk::connect()
{
    addToCommandBuffer([=]() -> void {
        if (m_connectivityService->connect())
        {
            m_connected = true;
            registerDevices();

            for (const auto& kvp : m_devices)
            {
                for (const std::string& actuatorReference : kvp.second.getActuatorReferences())
                {
                    publishActuatorStatus(kvp.first, actuatorReference);
                }
            }

            publish();
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
            connect();
        }
    });
}

void Wolk::disconnect()
{
    addToCommandBuffer([=]() -> void {
        m_connected = false;
        m_connectivityService->disconnect();
    });
}

void Wolk::publish()
{
    addToCommandBuffer([=]() -> void {
        m_dataService->publishActuatorStatuses();
        m_dataService->publishAlarms();
        m_dataService->publishSensorReadings();
    });
}

void Wolk::publish(const std::string& deviceKey)
{
    addToCommandBuffer([=]() -> void {
        if (!deviceExists(deviceKey))
        {
            LOG(ERROR) << "Device does not exist: " << deviceKey;
            return;
        }

        m_dataService->publishActuatorStatuses(deviceKey);
        m_dataService->publishAlarms(deviceKey);
        m_dataService->publishSensorReadings(deviceKey);
    });
}

void Wolk::addDevice(const Device& device)
{
    addToCommandBuffer([=] {
        const std::string deviceKey = device.getKey();
        if (deviceExists(deviceKey))
        {
            LOG(ERROR) << "Device with key '" << deviceKey << "' was already added";
            return;
        }

        m_devices[deviceKey] = device;

        m_deviceStatusService->devicesUpdated(getDeviceKeys());

        if (m_connected)
        {
            registerDevice(device);
            m_connectivityService->reconnect();
        }
    });
}

void Wolk::removeDevice(const std::string& deviceKey)
{
    addToCommandBuffer([=] {
        auto it = m_devices.find(deviceKey);
        if (it != m_devices.end())
        {
            m_devices.erase(it);
        }
    });
}

Wolk::Wolk() : m_connected{false}, m_commandBuffer{new CommandBuffer()} {}

Wolk::~Wolk()
{
    m_commandBuffer->stop();
}

void Wolk::addToCommandBuffer(std::function<void()> command)
{
    m_commandBuffer->pushCommand(std::make_shared<std::function<void()>>(command));
}

unsigned long long Wolk::currentRtc()
{
    auto duration = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::seconds>(duration).count());
}

void Wolk::handleActuatorSetCommand(const std::string& key, const std::string& reference, const std::string& value)
{
    addToCommandBuffer([=] {
        if (!deviceExists(key))
        {
            LOG(ERROR) << "Device does not exist: " << key;
            return;
        }

        if (!actuatorDefinedForDevice(key, reference))
        {
            LOG(ERROR) << "Actuator does not exist for device: " << key << ", " << reference;
            return;
        }

        if (m_actuationHandler)
        {
            m_actuationHandler->operator()(key, reference, value);
        }
        else if (m_actuationHandlerLambda)
        {
            m_actuationHandlerLambda(key, reference, value);
        }

        const ActuatorStatus actuatorStatus = [&] {
            if (m_actuatorStatusProvider)
            {
                return m_actuatorStatusProvider->operator()(key, reference);
            }
            else if (m_actuatorStatusProviderLambda)
            {
                return m_actuatorStatusProviderLambda(key, reference);
            }

            return ActuatorStatus("", ActuatorStatus::State::ERROR);
        }();

        m_dataService->addActuatorStatus(key, reference, actuatorStatus.getValue(), actuatorStatus.getState());
        m_dataService->publishActuatorStatuses();
    });
}

void Wolk::handleActuatorGetCommand(const std::string& key, const std::string& reference)
{
    addToCommandBuffer([=] {
        if (!deviceExists(key))
        {
            LOG(ERROR) << "Device does not exist: " << key;
            return;
        }

        if (!actuatorDefinedForDevice(key, reference))
        {
            LOG(ERROR) << "Actuator does not exist for device: " << key << ", " << reference;
            return;
        }

        const ActuatorStatus actuatorStatus = [&] {
            if (m_actuatorStatusProvider)
            {
                return m_actuatorStatusProvider->operator()(key, reference);
            }
            else if (m_actuatorStatusProviderLambda)
            {
                return m_actuatorStatusProviderLambda(key, reference);
            }

            return ActuatorStatus("", ActuatorStatus::State::ERROR);
        }();

        m_dataService->addActuatorStatus(key, reference, actuatorStatus.getValue(), actuatorStatus.getState());
        m_dataService->publishActuatorStatuses();
    });
}

void Wolk::handleDeviceStatusRequest(const std::string& key)
{
    addToCommandBuffer([=] {
        const DeviceStatus status = [&] {
            if (m_deviceStatusProvider)
            {
                return m_deviceStatusProvider->operator()(key);
            }
            else if (m_deviceStatusProviderLambda)
            {
                return m_deviceStatusProviderLambda(key);
            }

            return DeviceStatus::OFFLINE;
        }();

        m_deviceStatusService->publishDeviceStatus(key, status);
    });
}

void Wolk::registerDevice(const Device& device)
{
    addToCommandBuffer([=] { m_deviceRegistrationService->publishRegistrationRequest(device); });
}

void Wolk::registerDevices()
{
    addToCommandBuffer([=] {
        for (const auto& kvp : m_devices)
        {
            registerDevice(kvp.second);
        }
    });
}

std::vector<std::string> Wolk::getDeviceKeys()
{
    std::vector<std::string> keys;
    for (const auto& kvp : m_devices)
    {
        keys.push_back(kvp.first);
    }

    return keys;
}

bool Wolk::deviceExists(const std::string& deviceKey)
{
    auto it = m_devices.find(deviceKey);
    return it != m_devices.end();
}

bool Wolk::sensorDefinedForDevice(const std::string& deviceKey, const std::string& reference)
{
    auto it = m_devices.find(deviceKey);
    if (it == m_devices.end())
    {
        return false;
    }

    const auto sensors = it->second.getManifest().getSensors();
    auto sensorIt = std::find_if(sensors.begin(), sensors.end(),
                                 [&](const SensorManifest& manifest) { return manifest.getReference() == reference; });

    return sensorIt != sensors.end();
}

bool Wolk::alarmDefinedForDevice(const std::string& deviceKey, const std::string& reference)
{
    auto it = m_devices.find(deviceKey);
    if (it == m_devices.end())
    {
        return false;
    }

    const auto alarms = it->second.getManifest().getAlarms();
    auto alarmIt = std::find_if(alarms.begin(), alarms.end(),
                                [&](const AlarmManifest& manifest) { return manifest.getReference() == reference; });

    return alarmIt != alarms.end();
}

bool Wolk::actuatorDefinedForDevice(const std::string& deviceKey, const std::string& reference)
{
    auto it = m_devices.find(deviceKey);
    if (it == m_devices.end())
    {
        return false;
    }

    const auto actuators = it->second.getActuatorReferences();
    auto actuatorIt = std::find(actuators.begin(), actuators.end(), reference);

    return actuatorIt != actuators.end();
}

void Wolk::handleRegistrationResponse(const std::string& deviceKey, DeviceRegistrationResponse::Result result)
{
    LOG(INFO) << "Registration response for device '" << deviceKey << "' received: " << static_cast<int>(result);
}

Wolk::ConnectivityFacade::ConnectivityFacade(InboundMessageHandler& handler,
                                             std::function<void()> connectionLostHandler)
: m_messageHandler{handler}, m_connectionLostHandler{connectionLostHandler}
{
}

void Wolk::ConnectivityFacade::messageReceived(const std::string& channel, const std::string& message)
{
    m_messageHandler.messageReceived(channel, message);
}

void Wolk::ConnectivityFacade::connectionLost()
{
    m_connectionLostHandler();
}

std::vector<std::string> Wolk::ConnectivityFacade::getChannels() const
{
    return m_messageHandler.getChannels();
}
}    // namespace wolkabout
