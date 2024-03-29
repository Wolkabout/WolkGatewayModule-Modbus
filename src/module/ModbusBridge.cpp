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

#include "DeviceStatusProvider.h"
#include "RegisterMappingFactory.h"
#include "core/utilities/Logger.h"
#include "more_modbus/mappings/BoolMapping.h"
#include "more_modbus/modbus/ModbusClient.h"

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
const std::vector<std::string> ModbusBridge::TRUE_VALUES = {"true", "1", "1.0", "ON"};

ModbusBridge::ModbusBridge(
  ModbusClient& modbusClient,
  const std::map<std::string, std::unique_ptr<DevicesConfigurationTemplate>>& configurationTemplates,
  const std::map<std::string, std::vector<int>>& deviceAddressesByTemplate,
  const std::map<int, std::unique_ptr<Device>>& devices, std::chrono::milliseconds registerReadPeriod,
  std::unique_ptr<KeyValuePersistence> defaultValuePersistence,
  std::unique_ptr<KeyValuePersistence> repeatValuePersistence, std::unique_ptr<KeyValuePersistence> safeModePersistence)
: m_modbusClient(modbusClient)
, m_registerReadPeriod(registerReadPeriod)
, m_deviceKeyBySlaveAddress()
, m_registerMappingByReference()
, m_configurationMappingByDeviceKeyAndRef()
, m_connectivityStatus(ConnectivityStatus::NONE)
, m_defaultValuePersistence(std::move(defaultValuePersistence))
, m_repeatValuePersistence(std::move(repeatValuePersistence))
, m_safeModePersistence(std::move(safeModePersistence))
{
    std::vector<std::shared_ptr<ModbusDevice>> modbusDevices;
    auto defaultValues = m_defaultValuePersistence->loadValues();
    auto repeatedValues = m_repeatValuePersistence->loadValues();
    auto safeModeValues = m_safeModePersistence->loadValues();

    for (const auto& templateRegistered : deviceAddressesByTemplate)
    {
        // Create an initial list of mappings for the template. Mark all the mappings that will
        // be configuration mappings,
        const auto& configurationTemplate = *(configurationTemplates).at(templateRegistered.first);
        std::vector<std::shared_ptr<RegisterMapping>> mappings;
        std::map<std::string, std::string> defaultValueMappings;
        std::map<std::string, std::chrono::milliseconds> repeatValueMappings;
        std::map<std::string, std::string> safeMappings;
        std::map<std::string, ModuleMapping::MappingType> mappingTypeByReference;
        std::map<std::string, std::string> configurationKeysAndLabels;

        for (const auto& mapping : configurationTemplate.getMappings())
        {
            // Remember all the configurations for this template.
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
            mappingTypeByReference.emplace(mapping.getReference(), mapping.getMappingType());

            // If any of the mappings are in the special categories
            if (!mapping.getDefaultValue().empty())
                defaultValueMappings.emplace(mapping.getReference(), mapping.getDefaultValue());
            if (mapping.getRepeat().count() > 0)
                repeatValueMappings.emplace(mapping.getReference(), mapping.getRepeat());
            if (mapping.hasSafeMode())
                safeMappings.emplace(mapping.getReference(), mapping.getSafeModeValue());
        }

        // Foreach device slaveAddress, copy over the mappings to create the device.
        for (const auto& slaveAddress : templateRegistered.second)
        {
            const auto key = devices.at(slaveAddress)->getKey();

            // Filter out the default, repeat and safe mode values for this device
            const auto defaultValuesForDevice = [&]() {
                auto map = std::map<std::string, std::string>{};
                std::copy_if(defaultValues.cbegin(), defaultValues.cend(), std::inserter(map, map.end()),
                             [&](const std::pair<std::string, std::string>& pair) {
                                 return pair.first.find(key + SEPARATOR) != std::string::npos;
                             });
                return map;
            }();
            const auto repeatValuesForDevice = [&]() {
                auto map = std::map<std::string, std::string>{};
                std::copy_if(repeatedValues.cbegin(), repeatedValues.cend(), std::inserter(map, map.end()),
                             [&](const std::pair<std::string, std::string>& pair) {
                                 return pair.first.find(key + SEPARATOR) != std::string::npos;
                             });
                return map;
            }();
            const auto safeModeValueForDevice = [&]() {
                auto map = std::map<std::string, std::string>{};
                std::copy_if(safeModeValues.cbegin(), safeModeValues.cend(), std::inserter(map, map.end()),
                             [&](const std::pair<std::string, std::string>& pair) {
                                 return pair.first.find(key + SEPARATOR) != std::string::npos;
                             });
                return map;
            }();

            const auto device = std::make_shared<ModbusDevice>(key, slaveAddress);
            device->createGroups(mappings);

            modbusDevices.emplace_back(device);

            m_deviceKeyBySlaveAddress.emplace(slaveAddress, key);
            m_devicesStatusBySlaveAddress.emplace(slaveAddress, DeviceStatus::Status::OFFLINE);

            // Register all the mappings into a map, keep configuration mappings special too.
            for (const auto& group : device->getGroups())
            {
                for (const auto& mapping : group->getMappings())
                {
                    m_registerMappingByReference.emplace(key + SEPARATOR + mapping.second->getReference(),
                                                         mapping.second);
                    m_registerMappingTypeByReference.emplace(key + SEPARATOR + mapping.second->getReference(),
                                                             mappingTypeByReference[mapping.second->getReference()]);

                    const auto defaultValueIt = defaultValueMappings.find(mapping.second->getReference());
                    if (defaultValueIt != defaultValueMappings.cend())
                    {
                        auto defaultValue = defaultValueIt->second;
                        const auto it = defaultValuesForDevice.find(key + SEPARATOR + mapping.second->getReference());
                        if (it != defaultValuesForDevice.cend())
                            defaultValue = it->second;
                        m_defaultValueMappingByReference.emplace(key + SEPARATOR + mapping.second->getReference(),
                                                                 defaultValue);
                    }

                    const auto repeatIt = repeatValueMappings.find(mapping.second->getReference());
                    if (repeatIt != repeatValueMappings.cend())
                    {
                        auto repeatValue = repeatIt->second;
                        const auto it = repeatValuesForDevice.find(key + SEPARATOR + mapping.second->getReference());
                        if (it != repeatValuesForDevice.cend())
                        {
                            try
                            {
                                repeatValue = std::chrono::milliseconds(std::stoull(it->second));
                            }
                            catch (const std::exception& exception)
                            {
                                LOG(WARN) << "Found invalid persisted `repeat` value for '" << key << "'/'"
                                          << mapping.second->getReference() << "'.";
                            }
                        }
                        m_repeatedWriteMappingByReference.emplace(key + SEPARATOR + mapping.second->getReference(),
                                                                  repeatValue);

                        mapping.second->setRepeatedWrite(repeatValue);
                    }

                    const auto safeIt = safeMappings.find(mapping.second->getReference());
                    if (safeIt != safeMappings.cend())
                    {
                        auto safeModeValue = safeIt->second;
                        const auto it = safeModeValueForDevice.find(key + SEPARATOR + mapping.second->getReference());
                        if (it != safeModeValueForDevice.cend())
                            safeModeValue = it->second;
                        m_safeModeMappingByReference.emplace(key + SEPARATOR + mapping.second->getReference(),
                                                             safeModeValue);
                    }

                    const auto& it = configurationKeysAndLabels.find(mapping.second->getReference());
                    if (it != configurationKeysAndLabels.end())
                    {
                        const std::string& configurationReference = it->second;
                        const std::string& mapKey = key + SEPARATOR + configurationReference;

                        if (m_configurationMappingByDeviceKey.find(key) == m_configurationMappingByDeviceKey.end())
                        {
                            m_configurationMappingByDeviceKey.emplace(key, std::vector<std::string>());
                        }

                        m_configurationMappingByDeviceKey[key].emplace_back(mapKey);

                        if (m_configurationMappingByDeviceKeyAndRef.find(mapKey) ==
                            m_configurationMappingByDeviceKeyAndRef.end())
                        {
                            m_configurationMappingByDeviceKeyAndRef.emplace(
                              mapKey, std::map<std::string, std::shared_ptr<RegisterMapping>>());
                        }

                        m_configurationMappingByDeviceKeyAndRef[mapKey].emplace(mapping.second->getReference(),
                                                                                mapping.second);
                    }
                }
            }
        }
    }

    // Create the reader
    m_modbusReader = std::make_shared<ModbusReader>(m_modbusClient, m_registerReadPeriod);

    m_modbusReader->addDevices(modbusDevices);

    // Setup the device mapping value change logic.
    for (const auto& device : modbusDevices)
    {
        device->setOnStatusChange([this, device](bool status) {
            const auto newStatus = status ? DeviceStatus::Status::CONNECTED : DeviceStatus::Status::OFFLINE;

            if (m_devicesStatusBySlaveAddress[device->getSlaveAddress()] != newStatus)
            {
                m_devicesStatusBySlaveAddress[device->getSlaveAddress()] = newStatus;
                m_onDeviceStatusChange(device->getName(), newStatus);
            }
        });

        device->setOnMappingValueChange(
          [this, device](const std::shared_ptr<RegisterMapping>& mapping, const std::vector<uint16_t>& bytes) {
              switch (m_registerMappingTypeByReference.at(device->getName() + SEPARATOR + mapping->getReference()))
              {
              case ModuleMapping::MappingType::DEFAULT:
                  if (mapping->getRegisterType() == RegisterMapping::RegisterType::COIL ||
                      mapping->getRegisterType() == RegisterMapping::RegisterType::HOLDING_REGISTER)
                  {
                      m_onActuatorStatusChange(device->getName(), mapping->getReference(), readFromMapping(mapping));
                  }
                  else
                  {
                      m_onSensorChange(device->getName(), mapping->getReference(), readFromMapping(mapping));
                  }
                  break;
              case ModuleMapping::MappingType::SENSOR:
                  m_onSensorChange(device->getName(), mapping->getReference(), readFromMapping(mapping));
                  break;
              case ModuleMapping::MappingType::ACTUATOR:
                  m_onActuatorStatusChange(device->getName(), mapping->getReference(), readFromMapping(mapping));
                  break;
              case ModuleMapping::MappingType::ALARM:
                  m_onAlarmChange(device->getName(), mapping->getReference(),
                                  std::dynamic_pointer_cast<BoolMapping>(mapping)->getBoolValue());
                  break;
              case ModuleMapping::MappingType::CONFIGURATION:
                  m_onConfigurationChange(device->getName());
                  break;
              }

              if (m_devicesStatusBySlaveAddress[device->getSlaveAddress()] != DeviceStatus::Status::CONNECTED)
              {
                  m_devicesStatusBySlaveAddress[device->getSlaveAddress()] = DeviceStatus::Status::CONNECTED;
                  m_onDeviceStatusChange(device->getName(), DeviceStatus::Status::CONNECTED);
              }
          });

        device->setOnMappingValueChange([this, device](const std::shared_ptr<RegisterMapping>& mapping, bool data) {
            switch (m_registerMappingTypeByReference.at(device->getName() + SEPARATOR + mapping->getReference()))
            {
            case ModuleMapping::MappingType::DEFAULT:
                if (mapping->getRegisterType() == RegisterMapping::RegisterType::COIL ||
                    mapping->getRegisterType() == RegisterMapping::RegisterType::HOLDING_REGISTER)
                {
                    m_onActuatorStatusChange(device->getName(), mapping->getReference(), readFromMapping(mapping));
                }
                else
                {
                    m_onSensorChange(device->getName(), mapping->getReference(), readFromMapping(mapping));
                }
                break;
            case ModuleMapping::MappingType::SENSOR:
                m_onSensorChange(device->getName(), mapping->getReference(), readFromMapping(mapping));
                break;
            case ModuleMapping::MappingType::ACTUATOR:
                m_onActuatorStatusChange(device->getName(), mapping->getReference(), readFromMapping(mapping));
                break;
            case ModuleMapping::MappingType::ALARM:
                m_onAlarmChange(device->getName(), mapping->getReference(),
                                std::dynamic_pointer_cast<BoolMapping>(mapping)->getBoolValue());
                break;
            case ModuleMapping::MappingType::CONFIGURATION:
                m_onConfigurationChange(device->getName());
                break;
            }

            if (m_devicesStatusBySlaveAddress[device->getSlaveAddress()] != DeviceStatus::Status::CONNECTED)
            {
                m_devicesStatusBySlaveAddress[device->getSlaveAddress()] = DeviceStatus::Status::CONNECTED;
                m_onDeviceStatusChange(device->getName(), DeviceStatus::Status::CONNECTED);
            }
        });
    }
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

void ModbusBridge::setOnConfigurationChange(const std::function<void(const std::string&)>& onConfigurationChange)
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
    for (const auto& defaultValue : m_defaultValueMappingByReference)
    {
        const auto& mapping = m_registerMappingByReference[defaultValue.first];
        writeToMapping(mapping, defaultValue.second);

        if (auto group = mapping->getGroup().lock())
        {
            if (auto device = group->getDevice().lock())
            {
                if (mapping->getOutputType() == RegisterMapping::OutputType::BOOL)
                {
                    device->triggerOnMappingValueChange(mapping, mapping->getBoolValue());
                }
                else
                {
                    device->triggerOnMappingValueChange(mapping, mapping->getBytesValues());
                }
            }
        }
    }
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

void ModbusBridge::platformStatus(ConnectivityStatus status)
{
    LOG(TRACE) << METHOD_INFO;

    // Look if the status has changed
    if (m_connectivityStatus == status)
        return;
    m_connectivityStatus = status;

    // Go through all the safe mode mappings and write their safe mode values in
    if (m_connectivityStatus == ConnectivityStatus::OFFLINE)
    {
        // Go through all safe mode mappings and write them all in
        for (const auto& pair : m_safeModeMappingByReference)
        {
            LOG(DEBUG) << "Triggered SafeMode write for '" << pair.first << "'.";

            // Find the mapping
            const auto& mapping = m_registerMappingByReference[pair.first];

            // Now check out the mapping type, try to convert the value
            switch (mapping->getOutputType())
            {
            case RegisterMapping::OutputType::BOOL:
                if (pair.second == "true" || pair.second == "false")
                {
                    writeToBoolMapping(mapping, pair.second);
                    if (auto group = mapping->getGroup().lock())
                    {
                        if (auto device = group->getDevice().lock())
                            device->triggerOnMappingValueChange(mapping, mapping->getBoolValue());
                    }
                }
                else
                {
                    LOG(WARN) << "Failed to write in SafeMode value for '" << pair.first
                              << "' - not a valid boolean value.";
                }
                break;
            case RegisterMapping::OutputType::UINT16:
                writeToUInt16Mapping(mapping, pair.second);
                if (auto group = mapping->getGroup().lock())
                {
                    if (auto device = group->getDevice().lock())
                        device->triggerOnMappingValueChange(mapping, mapping->getBytesValues());
                }
                break;
            case RegisterMapping::OutputType::INT16:
                writeToInt16Mapping(mapping, pair.second);
                if (auto group = mapping->getGroup().lock())
                {
                    if (auto device = group->getDevice().lock())
                        device->triggerOnMappingValueChange(mapping, mapping->getBytesValues());
                }
                break;
            case RegisterMapping::OutputType::UINT32:
                writeToUInt32Mapping(mapping, pair.second);
                if (auto group = mapping->getGroup().lock())
                {
                    if (auto device = group->getDevice().lock())
                        device->triggerOnMappingValueChange(mapping, mapping->getBytesValues());
                }
                break;
            case RegisterMapping::OutputType::INT32:
                writeToInt32Mapping(mapping, pair.second);
                if (auto group = mapping->getGroup().lock())
                {
                    if (auto device = group->getDevice().lock())
                        device->triggerOnMappingValueChange(mapping, mapping->getBytesValues());
                }
                break;
            case RegisterMapping::OutputType::FLOAT:
                writeToFloatMapping(mapping, pair.second);
                if (auto group = mapping->getGroup().lock())
                {
                    if (auto device = group->getDevice().lock())
                        device->triggerOnMappingValueChange(mapping, mapping->getBytesValues());
                }
                break;
            case RegisterMapping::OutputType::STRING:
                writeToStringMapping(mapping, pair.second);
                if (auto group = mapping->getGroup().lock())
                {
                    if (auto device = group->getDevice().lock())
                        device->triggerOnMappingValueChange(mapping, mapping->getBytesValues());
                }
                break;
            }
        }
    }
}
}    // namespace wolkabout
