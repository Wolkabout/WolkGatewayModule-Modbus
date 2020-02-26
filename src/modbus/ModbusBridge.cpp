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

#include "ModbusBridge.h"
#include "ActuationHandlerPerDevice.h"
#include "ActuatorStatusProviderPerDevice.h"
#include "DeviceStatusProvider.h"
#include "modbus/ModbusClient.h"
#include "modbus/ModbusRegisterMapping.h"
#include "modbus/libmodbus/modbus.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"

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
ModbusBridge::ModbusBridge(ModbusClient& modbusClient,
                           std::map<std::string, std::unique_ptr<DevicesConfigurationTemplate>>& configurationTemplates,
                           std::map<std::string, std::vector<int>>& deviceAddressesByTemplate,
                           std::map<int, std::unique_ptr<Device>>& devices,
                           std::chrono::milliseconds registerReadPeriod)
: m_modbusClient(modbusClient)
, m_timeoutIterator(0)
, m_shouldReconnect(false)
, m_deviceKeyBySlaveAddress()
, m_registerGroupsBySlaveAddress()
, m_devicesStatusBySlaveAddress()
, m_registerMappingByReference()
, m_configurationMappingByDevice()
, m_registerReadPeriod(registerReadPeriod)
, m_readerShouldRun(false)
{
    // By deviceAddressesByTemplate, take all the templates that are in use. From configurationTemplates
    // take the necessary info about the registers of the template, and by that, template the ModbusRegisterGroups
    // accordingly, then when that is prepared, for each device of that specific template, create instances of
    // ModbusRegisterGroups for the device (mostly for the slaveAddress).

    for (auto const& templateRegistered : deviceAddressesByTemplate)
    {
        // Take the template, make groups for the template
        DevicesConfigurationTemplate& configurationTemplate = *(configurationTemplates[templateRegistered.first]);

        std::vector<ModbusRegisterGroup> templatesGroups;
        //        auto &firstMapping = configurationTemplate.getMappings()[0];
        //        int previousSlaveAddress = firstMapping.getSlaveAddress();
        //        ModbusRegisterMapping::RegisterType previousRegisterType = firstMapping.getRegisterType();
        //        ModbusRegisterMapping::DataType previousDataType = firstMapping.getDataType();
        //        int previousAddress = firstMapping.getAddress() - 1;
        //
        //        ModbusRegisterGroup group(previousSlaveAddress, previousRegisterType, previousDataType);
        //        templatesGroups.push_back(group);

        for (auto const& mapping : configurationTemplate.getMappings())
        {
            templatesGroups.emplace_back(std::make_shared<ModbusRegisterMapping>(mapping));
            //            if (static_cast<int>(mapping.getRegisterType()) ==
            //                static_cast<int>(previousRegisterType))
            //            {
            //                if (static_cast<int>(mapping.getDataType()) == static_cast<int>(previousDataType))
            //                {
            //                    if ((mapping.getAddress() + mapping.getRegisterCount() - 1) == previousAddress + 1)
            //                    {
            //                        previousSlaveAddress = mapping.getSlaveAddress();
            //                        previousRegisterType = mapping.getRegisterType();
            //                        previousDataType = mapping.getDataType();
            //                        previousAddress = mapping.getAddress();
            //
            //                        group.addRegister(std::make_shared<ModbusRegisterMapping>(mapping));
            //                        continue;
            //                    }
            //                }
            //            }
            //
            //            previousSlaveAddress = mapping.getSlaveAddress();
            //            previousRegisterType = mapping.getRegisterType();
            //            previousDataType = mapping.getDataType();
            //            previousAddress = mapping.getAddress();
            //
            //            ModbusRegisterGroup nextRegisterGroup(previousSlaveAddress, previousRegisterType,
            //            previousDataType);
            //            nextRegisterGroup.addRegister(std::make_shared<ModbusRegisterMapping>(mapping));
            //            templatesGroups.push_back(nextRegisterGroup);
        }

        // Go through devices registered by this template
        // and make their groups (by copying made groups and assinging slaveAddresses.
        for (auto const& slaveAddress : templateRegistered.second)
        {
            // Copy over the templates groups (deep copy, objects are recreated)
            std::vector<ModbusRegisterGroup> devicesGroups;

            m_deviceKeyBySlaveAddress.insert(
              std::pair<int, std::string>(slaveAddress, devices[slaveAddress]->getKey()));
            m_devicesStatusBySlaveAddress.insert(
              std::pair<int, DeviceStatus::Status>(slaveAddress, DeviceStatus::Status::OFFLINE));

            for (auto const& templateGroup : templatesGroups)
            {
                // Set slave address to the devices group.
                ModbusRegisterGroup deviceGroup(templateGroup);
                deviceGroup.setSlaveAddress(slaveAddress);

                for (auto const& mapping : deviceGroup.getRegisters())
                {
                    mapping->setSlaveAddress(slaveAddress);
                    auto deviceKey = devices[slaveAddress]->getKey();
                    auto mappingPair = m_registerMappingByReference
                                         .insert(std::pair<std::string, std::shared_ptr<ModbusRegisterMapping>>(
                                           deviceKey + '.' + mapping->getReference(), mapping))
                                         .first;

                    // If the mapping is configuration, add it to the log of configurations for devices.
                    if (mapping->getMappingType() == ModbusRegisterMapping::MappingType::CONFIGURATION)
                    {
                        if (m_configurationMappingByDevice.find(deviceKey) == m_configurationMappingByDevice.end())
                        {
                            m_configurationMappingByDevice.emplace(
                              deviceKey, std::map<std::string, std::shared_ptr<ModbusRegisterMapping>>());
                        }

                        m_configurationMappingByDevice[deviceKey].insert(
                          std::pair<std::string, std::shared_ptr<ModbusRegisterMapping>>(mapping->getReference(),
                                                                                         mappingPair->second));
                    }

                    deviceGroup.addRegister(mapping);
                }

                devicesGroups.insert(devicesGroups.end(), deviceGroup);
            }

            // Place the device slave address in memory with its ModbusRegisterGroups
            m_registerGroupsBySlaveAddress.insert(
              std::pair<int, std::vector<ModbusRegisterGroup>>(slaveAddress, devicesGroups));
        }
    }
}

ModbusBridge::~ModbusBridge()
{
    m_modbusClient.disconnect();
    stop();
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

// handleActuation and helper methods
// void handleActuatorForHoldingRegister() -
void ModbusBridge::handleActuationForHoldingRegister(ModbusRegisterMapping& mapping, const std::string& value)
{
    if (mapping.getDataType() == ModbusRegisterMapping::DataType::INT16)
    {
        auto typedValue = static_cast<short>(std::stoi(value));
        if (!m_modbusClient.writeHoldingRegister(static_cast<int>(mapping.getSlaveAddress()), mapping.getAddress(),
                                                 typedValue))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << mapping.getAddress() << " Value: " << value;
            mapping.setValid(false);
        }
    }
    else if (mapping.getDataType() == ModbusRegisterMapping::DataType::UINT16)
    {
        auto typedValue = static_cast<unsigned short>(std::stoul(value));
        if (!m_modbusClient.writeHoldingRegister(mapping.getSlaveAddress(), mapping.getAddress(), typedValue))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << mapping.getAddress() << " Value: " << value;
            mapping.setValid(false);
        }
    }
    else if (mapping.getDataType() == ModbusRegisterMapping::DataType::REAL32)
    {
        float typedValue = std::stof(value);
        if (!m_modbusClient.writeHoldingRegister(mapping.getSlaveAddress(), mapping.getAddress(), typedValue))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << mapping.getAddress() << " Value: " << value;
            mapping.setValid(false);
        }
    }
}
// void handleActuatorForCoil() -
void ModbusBridge::handleActuationForCoil(ModbusRegisterMapping& mapping, const std::string& value)
{
    std::vector<std::string> trueValues = {"true", "1", "1.0", "ON"};
    bool boolValue = std::find(trueValues.begin(), trueValues.end(), value) != trueValues.end();
    if (!m_modbusClient.writeCoil(mapping.getSlaveAddress(), mapping.getAddress(), boolValue))
    {
        LOG(ERROR) << "ModbusBridge: Unable to write coil value - Register address: " << mapping.getAddress()
                   << " Value: " << value;
        mapping.setValid(false);
    }
}
// void handleActuator() - decides on the handler for incoming actuation
void ModbusBridge::handleActuation(const std::string& deviceKey, const std::string& reference, const std::string& value)
{
    // Find the device if it exists, find the reference in his data, and change the value.
    LOG(INFO) << "ModbusBridge: Handling actuation on " << deviceKey << " reference '" << reference << "' - Value: '"
              << value << "'";

    int slaveAddress = getSlaveAddress(deviceKey);
    if (slaveAddress == -1 || m_deviceKeyBySlaveAddress.find(slaveAddress) == m_deviceKeyBySlaveAddress.end())
    {
        LOG(ERROR) << "ModbusBridge: No device with key '" << deviceKey << "'";
        return;
    }

    // After we found the device, check for the reference existence, and then the type of reference. Pass on to handler.
    if (m_registerMappingByReference.find(deviceKey + '.' + reference) == m_registerMappingByReference.end())
    {
        LOG(ERROR) << "ModbusBridge: Reference '" << reference << "' does not exist for device '" << deviceKey << "'";
        return;
    }

    auto mapping = *m_registerMappingByReference[deviceKey + '.' + reference];

    // Discard if the mapping is incorrect
    if (mapping.getRegisterType() != ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR &&
        mapping.getRegisterType() != ModbusRegisterMapping::RegisterType::COIL)
    {
        LOG(ERROR)
          << "ModbusBridge: Modbus register mapped to reference '" << reference
          << "' can not be treated as actuator - Modbus register must be of type 'HOLDING_REGISTER_ACTUATOR' or 'COIL'";
        return;
    }

    // Pass through the appropriate handler
    if (mapping.getRegisterType() == ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR)
    {
        handleActuationForHoldingRegister(mapping, value);
    }
    else if (mapping.getRegisterType() == ModbusRegisterMapping::RegisterType::COIL)
    {
        handleActuationForCoil(mapping, value);
    }
}

// handleConfiguration and helper methods
// handleConfigurationForHoldingRegister() - handles
void ModbusBridge::handleConfigurationForHoldingRegister(ModbusRegisterMapping& mapping, const std::string& value)
{
    if (mapping.getDataType() == ModbusRegisterMapping::DataType::INT16)
    {
        auto shortValue = static_cast<short>(std::stoi(value));
        if (!m_modbusClient.writeHoldingRegister(mapping.getSlaveAddress(), mapping.getAddress(), shortValue))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << mapping.getAddress() << " Value: " << shortValue;
            mapping.setValid(false);
        }
    }
    else if (mapping.getDataType() == ModbusRegisterMapping::DataType::UINT16)
    {
        auto uShortValue = static_cast<unsigned short>(std::stoi(value));
        if (!m_modbusClient.writeHoldingRegister(mapping.getSlaveAddress(), mapping.getAddress(), uShortValue))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << mapping.getAddress() << " Value: " << uShortValue;
            mapping.setValid(false);
        }
    }
    else if (mapping.getDataType() == ModbusRegisterMapping::DataType::REAL32)
    {
        auto floatValue = std::stof(value);
        if (!m_modbusClient.writeHoldingRegister(mapping.getSlaveAddress(), mapping.getAddress(), floatValue))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << mapping.getAddress() << " Value: " << floatValue;
            mapping.setValid(false);
        }
    }
}
// handleConfigurationForHoldingRegisters() - handles
void ModbusBridge::handleConfigurationForHoldingRegisters(ModbusRegisterMapping& mapping,
                                                          const std::vector<std::string>& value)
{
    if (mapping.getDataType() == ModbusRegisterMapping::DataType::INT16)
    {
        std::vector<short> shorts;
        shorts.reserve(value.size());
        for (const std::string& shortString : value)
        {
            shorts.push_back(static_cast<short>(std::stoi(shortString)));
        }
        if (!m_modbusClient.writeHoldingRegisters(mapping.getSlaveAddress(), mapping.getAddress(), shorts))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << mapping.getAddress();
            mapping.setValid(false);
        }
    }
    else if (mapping.getDataType() == ModbusRegisterMapping::DataType::UINT16)
    {
        std::vector<unsigned short> uShorts;
        uShorts.reserve(value.size());
        for (const std::string& shortString : value)
        {
            uShorts.push_back(static_cast<unsigned short>(std::stoi(shortString)));
        }
        if (!m_modbusClient.writeHoldingRegisters(mapping.getSlaveAddress(), mapping.getAddress(), uShorts))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << mapping.getAddress();
            mapping.setValid(false);
        }
    }
    else if (mapping.getDataType() == ModbusRegisterMapping::DataType::REAL32)
    {
        std::vector<float> floats;
        floats.reserve(value.size());
        for (const std::string& shortString : value)
        {
            floats.push_back(static_cast<float>(std::stof(shortString)));
        }
        if (!m_modbusClient.writeHoldingRegisters(mapping.getSlaveAddress(), mapping.getAddress(), floats))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << mapping.getAddress();
            mapping.setValid(false);
        }
    }
}
// handleConfigurationForCoil() - handles
void ModbusBridge::handleConfigurationForCoil(ModbusRegisterMapping& mapping, const std::string& value)
{
    std::vector<std::string> trueValues = {"true", "1", "1.0", "ON"};
    bool boolValue = std::find(trueValues.begin(), trueValues.end(), value) != trueValues.end();
    if (!m_modbusClient.writeCoil(mapping.getSlaveAddress(), mapping.getAddress(), boolValue))
    {
        LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                   << mapping.getAddress() << " Value: " << boolValue;
        mapping.setValid(false);
    }
}
// handleConfiguration() - decides on the handler to handle incoming data
void ModbusBridge::handleConfiguration(const std::string& deviceKey,
                                       const std::vector<ConfigurationItem>& configuration)
{
    for (auto& config : configuration)
    {
        const std::string& reference = config.getReference();
        auto mapping = *m_registerMappingByReference[deviceKey + '.' + reference];

        if (mapping.getRegisterType() == ModbusRegisterMapping::RegisterType::COIL)
        {
            handleConfigurationForCoil(mapping, config.getValues()[0]);
        }
        else
        {
            if (!mapping.getLabelsAndAddresses().empty())
            {
                handleConfigurationForHoldingRegisters(mapping, config.getValues());
            }
            else
            {
                handleConfigurationForHoldingRegister(mapping, config.getValues()[0]);
            }
        }
    }
}

// getActuatorStatus and helper methods
// getActuatorStatusFromHoldingRegister() - handles
wolkabout::ActuatorStatus ModbusBridge::getActuatorStatusFromHoldingRegister(ModbusRegisterMapping& mapping)
{
    if (mapping.getDataType() == ModbusRegisterMapping::DataType::INT16)
    {
        signed short value;
        if (!m_modbusClient.readHoldingRegister(mapping.getSlaveAddress(), mapping.getAddress(), value))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read holding register with address '" << mapping.getAddress() << "'";

            mapping.setValid(false);
            return ActuatorStatus("", ActuatorStatus::State::ERROR);
        }

        mapping.update(value);

        return ActuatorStatus(std::to_string(value), ActuatorStatus::State::READY);
    }
    else if (mapping.getDataType() == ModbusRegisterMapping::DataType::UINT16)
    {
        unsigned short value;
        if (!m_modbusClient.readHoldingRegister(mapping.getSlaveAddress(), mapping.getAddress(), value))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read holding register with address '" << mapping.getAddress() << "'";

            mapping.setValid(false);
            return ActuatorStatus("", ActuatorStatus::State::ERROR);
        }

        mapping.update(value);

        return ActuatorStatus(std::to_string(value), ActuatorStatus::State::READY);
    }
    else if (mapping.getDataType() == ModbusRegisterMapping::DataType::REAL32)
    {
        float value;
        if (!m_modbusClient.readHoldingRegister(mapping.getSlaveAddress(), mapping.getAddress(), value))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read holding register with address '" << mapping.getAddress() << "'";

            mapping.setValid(false);
            return ActuatorStatus("", ActuatorStatus::State::ERROR);
        }

        mapping.update(value);

        return ActuatorStatus(std::to_string(value), ActuatorStatus::State::READY);
    }

    LOG(ERROR) << "ModbusBridge: Could not obtain actuator status from holding register - Unsupported data type";
    return ActuatorStatus("", ActuatorStatus::State::ERROR);
}
// getActuatorStatusFromCoil() - handles
wolkabout::ActuatorStatus ModbusBridge::getActuatorStatusFromCoil(ModbusRegisterMapping& mapping)
{
    bool value = false;
    if (!m_modbusClient.readCoil(mapping.getSlaveAddress(), mapping.getAddress(), value))
    {
        LOG(ERROR) << "ModbusBridge: Unable to read coil with address '" << mapping.getAddress() << "'";

        mapping.setValid(false);
        return ActuatorStatus("", ActuatorStatus::State::ERROR);
    }

    mapping.update(value);

    return ActuatorStatus(value ? "true" : "false", ActuatorStatus::State::READY);
}
// getActuatorStatus() - decides on the handler to handle the status request
wolkabout::ActuatorStatus ModbusBridge::getActuatorStatus(const std::string& deviceKey, const std::string& reference)
{
    // Find the device if it exists, find the reference in his data, and change the value.
    LOG(INFO) << "ModbusBridge: Getting actuation status on " << deviceKey << " reference '" << reference << "'";

    int slaveAddress = getSlaveAddress(deviceKey);
    if (slaveAddress == -1 || m_deviceKeyBySlaveAddress.find(slaveAddress) == m_deviceKeyBySlaveAddress.end())
    {
        LOG(ERROR) << "ModbusBridge: No device with key '" << deviceKey << "'";
        return ActuatorStatus("", ActuatorStatus::State::ERROR);
    }

    // After we found the device, check for the reference existence, and then the type of reference. Pass on to handler.
    if (m_registerMappingByReference.find(deviceKey + '.' + reference) == m_registerMappingByReference.end())
    {
        LOG(ERROR) << "ModbusBridge: Reference '" << reference << "' does not exist for device '" << deviceKey << "'";
        return ActuatorStatus("", ActuatorStatus::State::ERROR);
    }

    auto mapping = *(m_registerMappingByReference[deviceKey + '.' + reference]);

    if (mapping.getRegisterType() != ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR &&
        mapping.getRegisterType() != ModbusRegisterMapping::RegisterType::COIL)
    {
        LOG(ERROR)
          << "ModbusBridge: Modbus register mapped to reference '" << reference
          << "' can not be treated as actuator - Modbus register must be of type 'HOLDING_REGISTER_ACTUATOR' or 'COIL'";
        return ActuatorStatus("", ActuatorStatus::State::ERROR);
    }

    return mapping.getRegisterType() == ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR ?
             getActuatorStatusFromHoldingRegister(mapping) :
             getActuatorStatusFromCoil(mapping);
}

// getConfiguration and helper methods
// getConfigurationStatusFromHoldingRegister() - handles
ConfigurationItem ModbusBridge::getConfigurationStatusFromHoldingRegister(ModbusRegisterMapping& mapping)
{
    std::vector<std::string> configurationValues;
    if (mapping.getDataType() == ModbusRegisterMapping::DataType::INT16)
    {
        if (!mapping.getLabelsAndAddresses().empty())
        {
            std::vector<short> values;
            if (!m_modbusClient.readHoldingRegisters(mapping.getSlaveAddress(), mapping.getAddress(),
                                                     static_cast<int>(mapping.getLabelsAndAddresses().size()), values))
            {
                LOG(ERROR) << "ModbusBridge: Unable to read holding registers from address '" << mapping.getAddress()
                           << "'";

                mapping.setValid(false);
                return ConfigurationItem(std::vector<std::string>(), "");
            }
            mapping.update(values);
        }
        else
        {
            short value;
            if (!m_modbusClient.readHoldingRegister(mapping.getSlaveAddress(), mapping.getAddress(), value))
            {
                LOG(ERROR) << "ModbusBridge: Unable to read holding registers from address '" << mapping.getAddress()
                           << "'";

                mapping.setValid(false);
                return ConfigurationItem(std::vector<std::string>(), "");
            }
            mapping.update(value);
        }
    }
    else if (mapping.getDataType() == ModbusRegisterMapping::DataType::UINT16)
    {
        if (!mapping.getLabelsAndAddresses().empty())
        {
            std::vector<unsigned short> values;
            if (!m_modbusClient.readHoldingRegisters(mapping.getSlaveAddress(), mapping.getAddress(),
                                                     static_cast<int>(mapping.getLabelsAndAddresses().size()), values))
            {
                LOG(ERROR) << "ModbusBridge: Unable to read holding registers from address '" << mapping.getAddress()
                           << "'";

                mapping.setValid(false);
                return ConfigurationItem(std::vector<std::string>(), "");
            }
            mapping.update(values);
        }
        else
        {
            unsigned short value;
            if (!m_modbusClient.readHoldingRegister(mapping.getSlaveAddress(), mapping.getAddress(), value))
            {
                LOG(ERROR) << "ModbusBridge: Unable to read holding registers from address '" << mapping.getAddress()
                           << "'";

                mapping.setValid(false);
                return ConfigurationItem(std::vector<std::string>(), "");
            }
            mapping.update(value);
        }
    }
    else if (mapping.getDataType() == ModbusRegisterMapping::DataType::REAL32)
    {
        if (!mapping.getLabelsAndAddresses().empty())
        {
            std::vector<float> values;
            if (!m_modbusClient.readHoldingRegisters(mapping.getSlaveAddress(), mapping.getAddress(),
                                                     static_cast<int>(mapping.getLabelsAndAddresses().size()), values))
            {
                LOG(ERROR) << "ModbusBridge: Unable to read holding registers from address '" << mapping.getAddress()
                           << "'";

                mapping.setValid(false);
                return ConfigurationItem(std::vector<std::string>(), "");
            }
            mapping.update(values);
        }
        else
        {
            float value;
            if (!m_modbusClient.readHoldingRegister(mapping.getSlaveAddress(), mapping.getAddress(), value))
            {
                LOG(ERROR) << "ModbusBridge: Unable to read holding registers from address '" << mapping.getAddress()
                           << "'";

                mapping.setValid(false);
                return ConfigurationItem(std::vector<std::string>(), "");
            }
            mapping.update(value);
        }
    }

    auto values = StringUtils::tokenize(mapping.getValue(), ",");
    for (auto& value : values)
    {
        configurationValues.emplace_back(value);
    }

    return ConfigurationItem(configurationValues, mapping.getReference());
}
// getConfigurationStatusFromCoil() - handles
ConfigurationItem ModbusBridge::getConfigurationStatusFromCoil(ModbusRegisterMapping& mapping)
{
    bool value;
    if (!m_modbusClient.readCoil(mapping.getSlaveAddress(), mapping.getAddress(), value))
    {
        LOG(ERROR) << "ModbusBridge: Unable to read coil on address '" << mapping.getAddress() << "'";

        mapping.setValid(false);
        return ConfigurationItem(std::vector<std::string>(), "");
    }
    mapping.update(value);
    return ConfigurationItem({mapping.getValue()}, mapping.getReference());
}
// getConfiguration() - decides on the handler to handle the status request
std::vector<ConfigurationItem> ModbusBridge::getConfiguration(const std::string& deviceKey)
{
    std::vector<ConfigurationItem> configurations;
    for (const auto& referenceMappingPair : m_configurationMappingByDevice[deviceKey])
    {
        auto& mapping = *referenceMappingPair.second;

        switch (mapping.getRegisterType())
        {
        case ModbusRegisterMapping::RegisterType::COIL:
            configurations.emplace_back(getConfigurationStatusFromCoil(mapping));
            break;
        case ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR:
            configurations.emplace_back(getConfigurationStatusFromHoldingRegister(mapping));
            break;
        default:
            throw std::logic_error("Invalid type of register for a configuration at reference '" +
                                   referenceMappingPair.first + "'!");
        }
    }

    return configurations;
}

// getDeviceStatus() - handles status request about a specific device
wolkabout::DeviceStatus::Status ModbusBridge::getDeviceStatus(const std::string& deviceKey)
{
    return m_devicesStatusBySlaveAddress[getSlaveAddress(deviceKey)];
}

// methods for the running logic of modbusBridge
void ModbusBridge::start()
{
    if (m_readerShouldRun)
    {
        return;
    }

    m_readerShouldRun = true;
    m_reader = std::unique_ptr<std::thread>(new std::thread(&ModbusBridge::run, this));
}
void ModbusBridge::stop()
{
    m_readerShouldRun = false;
    m_reader->join();
}
void ModbusBridge::run()
{
    m_shouldReconnect = true;
    m_timeoutIterator = 0;

    while (m_readerShouldRun)
    {
        if (!m_shouldReconnect)
        {
            readAndReportModbusRegistersValues();
        }
        else
        {
            // just disconnecting
            if (m_onDeviceStatusChange)
            {
                for (auto const& pair : m_deviceKeyBySlaveAddress)
                {
                    m_onDeviceStatusChange(pair.second, wolkabout::DeviceStatus::Status::OFFLINE);
                }
            }
            m_modbusClient.disconnect();

            // attempting to connect
            LOG(INFO) << "ModbusBridge: Attempting to connect";
            while (!m_modbusClient.connect())
            {
                std::this_thread::sleep_for(std::chrono::seconds(m_timeoutDurations[m_timeoutIterator]));
                if ((uint)m_timeoutIterator < m_timeoutDurations.size() - 1)
                {
                    m_timeoutIterator++;
                }
            }
            m_shouldReconnect = false;
            m_timeoutIterator = 0;

            if (m_onDeviceStatusChange)
            {
                for (auto const& pair : m_deviceKeyBySlaveAddress)
                {
                    m_onDeviceStatusChange(pair.second, wolkabout::DeviceStatus::Status::CONNECTED);
                }
            }
            readAndReportModbusRegistersValues();
        }

        std::this_thread::sleep_for(m_registerReadPeriod);
    }
}

// readAndReport - loop for reading all devices and their groups
// with helper methods, for reading specific type of group

void ModbusBridge::readHoldingRegistersActuators(const ModbusRegisterGroup& group, std::map<int, bool>& slavesRead)
{
    int slaveAddress = group.getSlaveAddress();
    std::string deviceKey = m_deviceKeyBySlaveAddress[slaveAddress];

    switch (group.getDataType())
    {
    case ModbusRegisterMapping::DataType::INT16:
    {
        std::vector<short> values;
        LOG(TRACE) << "readHoldingRegisters (INT16) " << group.getSlaveAddress() << ", "
                   << group.getStartingRegisterAddress() << ", " << group.getRegisterCount();
        if (!m_modbusClient.readHoldingRegisters(group.getSlaveAddress(),
                                                 static_cast<int>(group.getStartingRegisterAddress()),
                                                 static_cast<int>(group.getRegisterCount()), values))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read holding registers on slave " << group.getSlaveAddress()
                       << " from address '" << group.getStartingRegisterAddress() << "'";
            if (slavesRead.count(group.getSlaveAddress()) == 0)
            {
                slavesRead.insert(std::make_pair(group.getSlaveAddress(), false));
                m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::OFFLINE;
            }
            return;
        }

        slavesRead[group.getSlaveAddress()] = true;
        m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::CONNECTED;

        for (int i = 0, j = 0; i < group.getMappingsCount(); ++i)
        {
            auto mapping = group.getRegisters()[static_cast<uint>(i)];

            bool updated;
            // separate the values of group for each mapping they need to go to
            if (mapping->getLabelsAndAddresses().empty())
            {
                signed short value = values[static_cast<uint>(j++)];
                updated = mapping->update(value);
            }
            else
            {
                std::vector<short> groupValues;
                for (uint k = 0; k < mapping->getLabelsAndAddresses().size(); k++)
                {
                    groupValues.push_back(values[static_cast<uint>(j++)]);
                }
                updated = mapping->update(groupValues);
            }

            if (updated)
            {
                LOG(INFO) << "ModbusBridge: "
                          << ((mapping->getMappingType() == ModbusRegisterMapping::MappingType::CONFIGURATION) ?
                                "Configuration" :
                                "Actuator")
                          << " value changed - Reference: '" << mapping->getReference() << "' Value: '"
                          << mapping->getValue() << "'";

                switch (mapping->getMappingType())
                {
                case ModbusRegisterMapping::MappingType::DEFAULT:
                case ModbusRegisterMapping::MappingType::ACTUATOR:
                    if (m_onActuatorStatusChange)
                    {
                        m_onActuatorStatusChange(m_deviceKeyBySlaveAddress[group.getSlaveAddress()],
                                                 mapping->getReference(), mapping->getValue());
                    }
                    break;
                case ModbusRegisterMapping::MappingType::CONFIGURATION:
                    if (m_onConfigurationChange)
                    {
                        auto vector = std::vector<ConfigurationItem>{
                          ConfigurationItem(StringUtils::tokenize(mapping->getValue(), ","), mapping->getReference())};
                        m_onConfigurationChange(m_deviceKeyBySlaveAddress[group.getSlaveAddress()], vector);
                    }
                    break;
                default:
                    LOG(ERROR) << "This mapping wasn\'t declared properly!";
                    break;
                }
            }
        }
        break;
    }
    case ModbusRegisterMapping::DataType::UINT16:
    {
        std::vector<unsigned short> values;
        LOG(TRACE) << "readHoldingRegisters (UINT16) " << group.getSlaveAddress() << ", "
                   << group.getStartingRegisterAddress() << ", " << group.getRegisterCount();
        if (!m_modbusClient.readHoldingRegisters(group.getSlaveAddress(), group.getStartingRegisterAddress(),
                                                 group.getRegisterCount(), values))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read holding registers on slave " << group.getSlaveAddress()
                       << " from address '" << group.getStartingRegisterAddress() << "'";
            if (slavesRead.count(group.getSlaveAddress()) == 0)
            {
                slavesRead.insert(std::make_pair(group.getSlaveAddress(), false));
                m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::OFFLINE;
            }
            return;
        }

        slavesRead[group.getSlaveAddress()] = true;
        m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::CONNECTED;

        for (int i = 0, j = 0; i < group.getMappingsCount(); ++i)
        {
            auto mapping = group.getRegisters()[static_cast<uint>(i)];
            bool updated;
            // separate the values of group for each mapping they need to go to
            if (mapping->getLabelsAndAddresses().empty())
            {
                unsigned short value = values[static_cast<uint>(j++)];
                updated = mapping->update(value);
            }
            else
            {
                std::vector<unsigned short> groupValues;
                for (uint k = 0; k < mapping->getLabelsAndAddresses().size(); k++)
                {
                    groupValues.push_back(values[static_cast<uint>(j++)]);
                }
                updated = mapping->update(groupValues);
            }
            if (updated)
            {
                LOG(INFO) << "ModbusBridge: Actuator value changed - Reference: '" << mapping->getReference()
                          << "' Value: '" << mapping->getValue() << "'";

                switch (mapping->getMappingType())
                {
                case ModbusRegisterMapping::MappingType::DEFAULT:
                case ModbusRegisterMapping::MappingType::ACTUATOR:
                    if (m_onActuatorStatusChange)
                    {
                        m_onActuatorStatusChange(deviceKey, mapping->getReference(), mapping->getValue());
                    }
                    break;
                case ModbusRegisterMapping::MappingType::CONFIGURATION:
                    if (m_onConfigurationChange)
                    {
                        auto vector = std::vector<ConfigurationItem>{
                          ConfigurationItem(StringUtils::tokenize(mapping->getValue(), ","), mapping->getReference())};
                        m_onConfigurationChange(deviceKey, vector);
                    }
                    break;
                default:
                    LOG(ERROR) << "This mapping wasn\'t declared properly!";
                    break;
                }
            }
        }
        break;
    }
    case ModbusRegisterMapping::DataType::REAL32:
    {
        std::vector<float> values;
        LOG(TRACE) << "readHoldingRegisters (REAL32) " << group.getSlaveAddress() << ", "
                   << group.getStartingRegisterAddress() << ", " << group.getRegisterCount();
        if (!m_modbusClient.readHoldingRegisters(group.getSlaveAddress(), group.getStartingRegisterAddress(),
                                                 group.getRegisterCount(), values))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read holding registers on slave " << group.getSlaveAddress()
                       << " from address '" << group.getStartingRegisterAddress() << "'";
            if (slavesRead.count(group.getSlaveAddress()) == 0)
            {
                slavesRead.insert(std::make_pair(group.getSlaveAddress(), false));
                m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::OFFLINE;
            }
            return;
        }

        slavesRead[group.getSlaveAddress()] = true;
        m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::CONNECTED;

        for (int i = 0, j = 0; i < group.getMappingsCount(); ++i)
        {
            auto mapping = group.getRegisters()[static_cast<uint>(i)];

            bool updated;
            // separate the values of group for each mapping they need to go to
            if (mapping->getLabelsAndAddresses().empty())
            {
                float value = values[static_cast<uint>(j++)];
                updated = mapping->update(value);
            }
            else
            {
                std::vector<float> groupValues;
                for (uint k = 0; k < mapping->getLabelsAndAddresses().size(); k++)
                {
                    groupValues.push_back(values[static_cast<uint>(j++)]);
                }
                updated = mapping->update(groupValues);
            }
            if (updated)
            {
                LOG(INFO) << "ModbusBridge: Actuator value changed - Reference: '" << mapping->getReference()
                          << "' Value: '" << mapping->getValue() << "'";

                switch (mapping->getMappingType())
                {
                case ModbusRegisterMapping::MappingType::DEFAULT:
                case ModbusRegisterMapping::MappingType::ACTUATOR:
                    if (m_onActuatorStatusChange)
                    {
                        m_onActuatorStatusChange(deviceKey, mapping->getReference(), mapping->getValue());
                    }
                    break;
                case ModbusRegisterMapping::MappingType::CONFIGURATION:
                    if (m_onConfigurationChange)
                    {
                        auto vector = std::vector<ConfigurationItem>{
                          ConfigurationItem(StringUtils::tokenize(mapping->getValue(), ","), mapping->getReference())};
                        m_onConfigurationChange(deviceKey, vector);
                    }
                    break;
                default:
                    LOG(ERROR) << "This mapping wasn\'t declared properly!";
                    break;
                }
            }
        }
        break;
    }
    case ModbusRegisterMapping::DataType::BOOL:
        throw std::logic_error(&"Invalid type for HoldingRegisterActuator at slaveAddress "[group.getSlaveAddress()]);
    }
}

void ModbusBridge::readHoldingRegistersSensors(const ModbusRegisterGroup& group, std::map<int, bool>& slavesRead)
{
    int slaveAddress = group.getSlaveAddress();
    std::string deviceKey = m_deviceKeyBySlaveAddress[slaveAddress];

    switch (group.getDataType())
    {
    case ModbusRegisterMapping::DataType::INT16:
    {
        std::vector<signed short> values;
        LOG(TRACE) << "readHoldingRegisters (INT16) " << group.getSlaveAddress() << ", "
                   << group.getStartingRegisterAddress() << ", " << group.getRegisterCount();
        if (!m_modbusClient.readHoldingRegisters(group.getSlaveAddress(), group.getStartingRegisterAddress(),
                                                 group.getRegisterCount(), values))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read holding registers on slave " << group.getSlaveAddress()
                       << " from address '" << group.getStartingRegisterAddress() << "'";
            if (slavesRead.count(group.getSlaveAddress()) == 0)
            {
                slavesRead.insert(std::make_pair(group.getSlaveAddress(), false));
                m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::OFFLINE;
            }
            return;
        }

        slavesRead[group.getSlaveAddress()] = true;
        m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::CONNECTED;

        for (int i = 0; i < group.getMappingsCount(); ++i)
        {
            signed short value = values[static_cast<uint>(i)];
            auto mapping = group.getRegisters()[static_cast<uint>(i)];
            if (mapping->update(value))
            {
                LOG(INFO) << "ModbusBridge: Sensor value - Reference: '" << mapping->getReference() << "' Value: '"
                          << mapping->getValue() << "'";

                // This here isn't edited because the holding_register_sensor can't be anything else than a
                // sensor!
                if (m_onSensorChange)
                {
                    m_onSensorChange(deviceKey, mapping->getReference(), mapping->getValue());
                }
            }
        }
        break;
    }
    case ModbusRegisterMapping::DataType::UINT16:
    {
        std::vector<unsigned short> values;
        LOG(TRACE) << "readHoldingRegisters (UINT16) " << group.getSlaveAddress() << ", "
                   << group.getStartingRegisterAddress() << ", " << group.getRegisterCount();
        if (!m_modbusClient.readHoldingRegisters(group.getSlaveAddress(), group.getStartingRegisterAddress(),
                                                 group.getRegisterCount(), values))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read holding registers on slave " << group.getSlaveAddress()
                       << " from address '" << group.getStartingRegisterAddress() << "'";
            if (slavesRead.count(group.getSlaveAddress()) == 0)
            {
                slavesRead.insert(std::make_pair(group.getSlaveAddress(), false));
                m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::OFFLINE;
            }
            return;
        }

        slavesRead[group.getSlaveAddress()] = true;
        m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::CONNECTED;

        for (int i = 0; i < group.getMappingsCount(); ++i)
        {
            unsigned short value = values[static_cast<uint>(i)];
            auto mapping = group.getRegisters()[static_cast<uint>(i)];
            if (mapping->update(value))
            {
                LOG(INFO) << "ModbusBridge: Sensor value - Reference: '" << mapping->getReference() << "' Value: '"
                          << mapping->getValue() << "'";

                // This here isn't edited because the holding_register_sensor can't be anything else than a
                // sensor!
                if (m_onSensorChange)
                {
                    m_onSensorChange(deviceKey, mapping->getReference(), mapping->getValue());
                }
            }
        }
        break;
    }
    case ModbusRegisterMapping::DataType::REAL32:
    {
        std::vector<float> values;
        LOG(TRACE) << "readHoldingRegisters (REAL32) " << group.getSlaveAddress() << ", "
                   << group.getStartingRegisterAddress() << ", " << group.getRegisterCount();
        if (!m_modbusClient.readHoldingRegisters(group.getSlaveAddress(), group.getStartingRegisterAddress(),
                                                 group.getRegisterCount(), values))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read holding registers on slave " << group.getSlaveAddress()
                       << " from address '" << group.getStartingRegisterAddress() << "'";
            if (slavesRead.count(group.getSlaveAddress()) == 0)
            {
                slavesRead.insert(std::make_pair(group.getSlaveAddress(), false));
                m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::OFFLINE;
            }
            return;
        }

        slavesRead[group.getSlaveAddress()] = true;
        m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::CONNECTED;

        for (int i = 0; i < group.getMappingsCount(); ++i)
        {
            float value = values[static_cast<uint>(i)];
            auto mapping = group.getRegisters()[static_cast<uint>(i)];
            if (mapping->update(value))
            {
                LOG(INFO) << "ModbusBridge: Sensor value - Reference: '" << mapping->getReference() << "' Value: '"
                          << mapping->getValue() << "'";

                // This here isn't edited because the holding_register_sensor can't be anything else than a
                // sensor!
                if (m_onSensorChange)
                {
                    m_onSensorChange(deviceKey, mapping->getReference(), mapping->getValue());
                }
            }
        }
        break;
    }
    case ModbusRegisterMapping::DataType::BOOL:
        throw std::logic_error(&"Invalid type for HoldingRegisterSensor at slaveAddress "[group.getSlaveAddress()]);
    }
}

void ModbusBridge::readInputRegisters(const ModbusRegisterGroup& group, std::map<int, bool>& slavesRead)
{
    int slaveAddress = group.getSlaveAddress();
    std::string deviceKey = m_deviceKeyBySlaveAddress[slaveAddress];

    switch (group.getDataType())
    {
    case ModbusRegisterMapping::DataType::INT16:
    {
        std::vector<signed short> values;
        LOG(TRACE) << "readInputRegisters (INT16) " << group.getSlaveAddress() << ", "
                   << group.getStartingRegisterAddress() << ", " << group.getRegisterCount();
        if (!m_modbusClient.readInputRegisters(group.getSlaveAddress(), group.getStartingRegisterAddress(),
                                               group.getRegisterCount(), values))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read input registers on slave " << group.getSlaveAddress()
                       << " from address '" << group.getStartingRegisterAddress() << "'";
            if (slavesRead.count(group.getSlaveAddress()) == 0)
            {
                slavesRead.insert(std::make_pair(group.getSlaveAddress(), false));
                m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::OFFLINE;
            }
            return;
        }

        slavesRead[group.getSlaveAddress()] = true;
        m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::CONNECTED;

        for (int i = 0; i < group.getMappingsCount(); ++i)
        {
            signed short value = values[static_cast<uint>(i)];
            auto mapping = group.getRegisters()[static_cast<uint>(i)];
            if (mapping->update(value))
            {
                LOG(INFO) << "ModbusBridge: Sensor value - Reference: '" << mapping->getReference() << "' Value: '"
                          << mapping->getValue() << "'";

                // This here isn't edited because the input_register can't be anything else than a sensor!
                if (m_onSensorChange)
                {
                    m_onSensorChange(deviceKey, mapping->getReference(), mapping->getValue());
                }
            }
        }
        break;
    }
    case ModbusRegisterMapping::DataType::UINT16:
    {
        std::vector<unsigned short> values;
        LOG(TRACE) << "readInputRegisters (UINT16) " << group.getSlaveAddress() << ", "
                   << group.getStartingRegisterAddress() << ", " << group.getRegisterCount();
        if (!m_modbusClient.readInputRegisters(group.getSlaveAddress(), group.getStartingRegisterAddress(),
                                               group.getRegisterCount(), values))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read input registers on slave " << group.getSlaveAddress()
                       << " from address '" << group.getStartingRegisterAddress() << "'";
            if (slavesRead.count(group.getSlaveAddress()) == 0)
            {
                slavesRead.insert(std::make_pair(group.getSlaveAddress(), false));
                m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::OFFLINE;
            }
            return;
        }

        slavesRead[group.getSlaveAddress()] = true;
        m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::CONNECTED;

        for (int i = 0; i < group.getMappingsCount(); ++i)
        {
            unsigned short value = values[static_cast<uint>(i)];
            auto mapping = group.getRegisters()[static_cast<uint>(i)];
            if (mapping->update(value))
            {
                LOG(INFO) << "ModbusBridge: Sensor value - Reference: '" << mapping->getReference() << "' Value: '"
                          << mapping->getValue() << "'";

                // This here isn't edited because the input_register can't be anything else than a sensor!
                if (m_onSensorChange)
                {
                    m_onSensorChange(deviceKey, mapping->getReference(), mapping->getValue());
                }
            }
        }
        break;
    }
    case ModbusRegisterMapping::DataType::REAL32:
    {
        std::vector<float> values;
        LOG(TRACE) << "readInputRegisters (REAL32) " << group.getSlaveAddress() << ", "
                   << group.getStartingRegisterAddress() << ", " << group.getRegisterCount();
        if (!m_modbusClient.readInputRegisters(group.getSlaveAddress(), group.getStartingRegisterAddress(),
                                               group.getRegisterCount(), values))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read input registers on slave " << group.getSlaveAddress()
                       << " from address '" << group.getStartingRegisterAddress() << "'";
            if (slavesRead.count(group.getSlaveAddress()) == 0)
            {
                slavesRead.insert(std::make_pair(group.getSlaveAddress(), false));
                m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::OFFLINE;
            }
            return;
        }

        slavesRead[group.getSlaveAddress()] = true;
        m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::CONNECTED;

        for (int i = 0; i < group.getMappingsCount(); ++i)
        {
            float value = values[static_cast<uint>(i)];
            auto mapping = group.getRegisters()[static_cast<uint>(i)];
            if (mapping->update(value))
            {
                LOG(INFO) << "ModbusBridge: Sensor value - Reference: '" << mapping->getReference() << "' Value: '"
                          << mapping->getValue() << "'";

                // This here isn't edited because the input_register can't be anything else than a sensor!
                if (m_onSensorChange)
                {
                    m_onSensorChange(deviceKey, mapping->getReference(), mapping->getValue());
                }
            }
        }
        break;
    }
    case ModbusRegisterMapping::DataType::BOOL:
        throw std::logic_error(&"Invalid type for HoldingRegisterActuator at slaveAddress "[group.getSlaveAddress()]);
    }
}

void ModbusBridge::readCoils(const ModbusRegisterGroup& group, std::map<int, bool>& slavesRead)
{
    int slaveAddress = group.getSlaveAddress();
    std::string deviceKey = m_deviceKeyBySlaveAddress[slaveAddress];

    std::vector<bool> values;
    LOG(TRACE) << "readCoils " << group.getSlaveAddress() << ", " << group.getStartingRegisterAddress() << ", "
               << group.getRegisterCount();
    if (!m_modbusClient.readCoils(group.getSlaveAddress(), group.getStartingRegisterAddress(), group.getRegisterCount(),
                                  values))
    {
        LOG(ERROR) << "ModbusBridge: Unable to read coils on slave " << group.getSlaveAddress() << " from address '"
                   << group.getStartingRegisterAddress() << "'";

        if (slavesRead.count(group.getSlaveAddress()) == 0)
        {
            slavesRead.insert(std::make_pair(group.getSlaveAddress(), false));
            m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::OFFLINE;
        }
        return;
    }

    slavesRead[group.getSlaveAddress()] = true;
    m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::CONNECTED;

    for (uint i = 0; i < group.getMappingsCount(); ++i)
    {
        bool value = values[i];
        auto mapping = group.getRegisters()[i];
        if (mapping->update(value))
        {
            LOG(INFO) << "ModbusBridge: Actuator value changed - Reference: '" << mapping->getReference()
                      << "' Value: '" << mapping->getValue() << "'";

            switch (mapping->getMappingType())
            {
            case ModbusRegisterMapping::MappingType::DEFAULT:
            case ModbusRegisterMapping::MappingType::ACTUATOR:
                if (m_onActuatorStatusChange)
                {
                    m_onActuatorStatusChange(deviceKey, mapping->getReference(), mapping->getValue());
                }
                break;
            case ModbusRegisterMapping::MappingType::CONFIGURATION:
                if (m_onConfigurationChange)
                {
                    auto vector = std::vector<ConfigurationItem>{
                      ConfigurationItem(StringUtils::tokenize(mapping->getValue(), ","), mapping->getReference())};
                    m_onConfigurationChange(deviceKey, vector);
                }
                break;
            default:
                LOG(ERROR) << "Mapping " << mapping->getName() << " (address: " << mapping->getAddress() << ") "
                           << "can't be that mapping type!";
                break;
            }
        }
    }
}

void ModbusBridge::readDiscreteInputs(const ModbusRegisterGroup& group, std::map<int, bool>& slavesRead)
{
    int slaveAddress = group.getSlaveAddress();
    std::string deviceKey = m_deviceKeyBySlaveAddress[slaveAddress];

    std::vector<bool> values;
    LOG(TRACE) << "readInputContacts " << group.getSlaveAddress() << ", " << group.getStartingRegisterAddress() << ", "
               << group.getRegisterCount();
    if (!m_modbusClient.readInputContacts(group.getSlaveAddress(), group.getStartingRegisterAddress(),
                                          group.getRegisterCount(), values))
    {
        LOG(ERROR) << "ModbusBridge: Unable to read input contacts on slave " << group.getSlaveAddress()
                   << " from address '" << group.getStartingRegisterAddress() << "'";
        if (slavesRead.count(group.getSlaveAddress()) == 0)
        {
            slavesRead.insert(std::make_pair(group.getSlaveAddress(), false));
            m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::OFFLINE;
        }
        return;
    }

    slavesRead[group.getSlaveAddress()] = true;
    m_devicesStatusBySlaveAddress[group.getSlaveAddress()] = DeviceStatus::Status::CONNECTED;

    for (int i = 0; i < group.getMappingsCount(); ++i)
    {
        bool value = values[static_cast<uint>(i)];
        auto mapping = group.getRegisters()[static_cast<uint>(i)];
        if (mapping->update(value))
        {
            LOG(INFO) << "ModbusBridge: Sensor value - Reference: '" << mapping->getReference() << "' Value: '"
                      << mapping->getValue() << "'";

            switch (mapping->getMappingType())
            {
            case ModbusRegisterMapping::MappingType::DEFAULT:
            case ModbusRegisterMapping::MappingType::SENSOR:
                if (m_onSensorChange)
                {
                    m_onSensorChange(deviceKey, mapping->getReference(), mapping->getValue());
                }
                break;
            case ModbusRegisterMapping::MappingType::ALARM:
                if (m_onAlarmChange)
                {
                    m_onAlarmChange(deviceKey, mapping->getReference(), value);
                }
                break;
            default:
                LOG(ERROR) << "This mapping wasn\'t declared properly!";
                break;
            }
        }
    }
}

void ModbusBridge::readAndReportModbusRegistersValues()
{
    LOG(DEBUG) << "ModbusBridge: Reading and reporting register values";

    std::map<int, bool> slavesRead;

    for (auto const& pair : m_registerGroupsBySlaveAddress)
    {
        LOG(DEBUG) << "ModbusBridge: Device slave address : " << pair.first;

        for (auto const& group : pair.second)
        {
            switch (group.getRegisterType())
            {
            case ModbusRegisterMapping::RegisterType::COIL:
            {
                readCoils(group, slavesRead);
                break;
            }
            case ModbusRegisterMapping::RegisterType::INPUT_CONTACT:
            {
                readDiscreteInputs(group, slavesRead);
                break;
            }
            case ModbusRegisterMapping::RegisterType::INPUT_REGISTER:
            {
                readInputRegisters(group, slavesRead);
                break;
            }
            case ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_SENSOR:
            {
                readHoldingRegistersSensors(group, slavesRead);
                break;
            }
            case ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR:
            {
                readHoldingRegistersActuators(group, slavesRead);
                break;
            }
            }
        }
    }

    if (slavesRead.size() == 1)
    {
        m_shouldReconnect = !slavesRead.begin()->second;
    }
}
}    // namespace wolkabout
