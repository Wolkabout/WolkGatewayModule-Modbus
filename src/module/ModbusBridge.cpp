/*
 * Copyright 2020 WolkAbout Technology s.r.o.
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

#include "ModbusBridge.h"
#include "ActuationHandlerPerDevice.h"
#include "ActuatorStatusProviderPerDevice.h"
#include "DeviceStatusProvider.h"
#include "RegisterMappingFactory.h"
#include "modbus/ModbusClient.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace wolkabout
{
const char ModbusBridge::SEPARATOR = '.';

ModbusBridge::ModbusBridge(
  ModbusClient& modbusClient,
  const std::map<std::string, std::unique_ptr<DevicesConfigurationTemplate>>& configurationTemplates,
  const std::map<std::string, std::vector<int>>& deviceAddressesByTemplate,
  const std::map<int, std::unique_ptr<Device>>& devices, std::chrono::milliseconds registerReadPeriod)
: m_modbusClient(modbusClient)
, m_registerReadPeriod(registerReadPeriod)
, m_deviceKeyBySlaveAddress()
, m_registerMappingByReference()
, m_configurationMappingByDevice()
{
    std::vector<std::shared_ptr<ModbusDevice>> modbusDevices;

    // Create a modbusDevice from template
    // This includes creating a RegisterMapping from each JSON mapping.

    for (const auto& templateRegistered : deviceAddressesByTemplate)
    {
        const auto& configurationTemplate = *(configurationTemplates).at(templateRegistered.first);
        std::vector<std::shared_ptr<RegisterMapping>> mappings;
        std::map<std::string, std::string> configurationKeysAndLabels;

        for (const auto& mapping : configurationTemplate.getMappings())
        {
            if (mapping.getMappingType() == ModuleMapping::MappingType::CONFIGURATION)
            {
                if (!mapping.getLabelMap().empty())
                {
                    for (const auto& label : mapping.getLabelMap())
                    {
                        configurationKeysAndLabels.emplace(label.first, mapping.getReference());
                    }
                }
                else
                {
                    configurationKeysAndLabels.emplace(mapping.getReference(), mapping.getReference());
                }
            }

            mappings.emplace_back(RegisterMappingFactory::fromJSONMapping(mapping));
        }

        for (const auto& slaveAddress : templateRegistered.second)
        {
            const std::string& key = devices.at(slaveAddress)->getKey();
            const auto& device = std::make_shared<ModbusDevice>(key, slaveAddress, mappings);

            m_deviceKeyBySlaveAddress.emplace(slaveAddress, key);
            m_devicesStatusBySlaveAddress.emplace(slaveAddress, DeviceStatus::Status::OFFLINE);

            for (const auto& group : device->getGroups())
            {
                for (const auto& mapping : group->getMappings())
                {
                    m_registerMappingByReference.emplace(key + SEPARATOR + mapping->getReference(), mapping);

                    const auto& it = configurationKeysAndLabels.find(mapping->getReference());
                    if (it != configurationKeysAndLabels.end())
                    {
                        const std::string& configurationReference = it->second;
                        const std::string& mapKey = key + SEPARATOR + configurationReference;

                        if (m_configurationMappingByDevice.find(mapKey) == m_configurationMappingByDevice.end())
                        {
                            m_configurationMappingByDevice.emplace(
                              mapKey, std::map<std::string, std::shared_ptr<RegisterMapping>>());
                        }

                        m_configurationMappingByDevice[mapKey].emplace(mapping->getReference(), mapping);
                    }
                }
            }
        }
    }

    m_modbusReader =
      std::unique_ptr<ModbusReader>(new ModbusReader(m_modbusClient, modbusDevices, m_registerReadPeriod));

    m_modbusReader->setOnIterationStatuses([&](std::map<int8_t, bool> statuses) {
        for (const auto& pair : statuses)
        {
            m_devicesStatusBySlaveAddress[pair.first] =
              pair.second ? DeviceStatus::Status::CONNECTED : DeviceStatus::Status::OFFLINE;
        }
    });
}

ModbusBridge::~ModbusBridge()
{
    m_modbusClient.disconnect();
    stop();
}

bool ModbusBridge::isRunning() const
{
    return m_modbusReader->isRunning();
}

int ModbusBridge::getSlaveAddress(const std::string& deviceKey)
{
    auto iterator =
      std::find_if(m_deviceKeyBySlaveAddress.begin(), m_deviceKeyBySlaveAddress.end(),
                   [&](const std::pair<int, std::string>& element) { return element.second == deviceKey; });

    if (iterator == m_deviceKeyBySlaveAddress.end())
    {
        return -1;
    }
    return iterator->first;
}

// Setters for necessary callbacks, for sending data to Wolk when register change has been noticed.

void ModbusBridge::setOnSensorChange(
  const std::function<void(const std::string&, const std::string&, const std::string&)>& onSensorChange)
{
    m_onSensorChange = onSensorChange;
}

void ModbusBridge::setOnActuatorStatusChange(
  const std::function<void(const std::string&, const std::string&, const std::string&)>& onActuatorStatusChange)
{
    m_onActuatorStatusChange = onActuatorStatusChange;
}

void ModbusBridge::setOnAlarmChange(
  const std::function<void(const std::string&, const std::string&, bool)>& onAlarmChange)
{
    m_onAlarmChange = onAlarmChange;
}

void ModbusBridge::setOnConfigurationChange(
  const std::function<void(const std::string&, std::vector<ConfigurationItem>&)>& onConfigurationChange)
{
    m_onConfigurationChange = onConfigurationChange;
}

void ModbusBridge::setOnDeviceStatusChange(
  const std::function<void(const std::string&, wolkabout::DeviceStatus::Status)>& onDeviceStatusChange)
{
    m_onDeviceStatusChange = onDeviceStatusChange;
}

// methods for the running logic of modbusBridge
void ModbusBridge::start()
{
    m_modbusReader->start();
}

void ModbusBridge::stop()
{
    m_modbusReader->stop();

    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    for (const auto& device : m_deviceKeyBySlaveAddress)
    {
        m_onDeviceStatusChange(device.second, DeviceStatus::Status::OFFLINE);
    }
}
}    // namespace wolkabout
