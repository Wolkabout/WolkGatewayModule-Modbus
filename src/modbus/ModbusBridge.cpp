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
, m_deviceKeyBySlaveAddress()
, m_registerGroupsBySlaveAddress()
, m_devicesStatusBySlaveAddress()
, m_registerWatcherByReference()
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

        // TODO Improve logic for creating modbusRegisterGroups - copy over the old one and adjust it
        for (auto const& mapping : configurationTemplate.getMappings())
        {
            templatesGroups.emplace_back(mapping);
        }

        // Go through devices registered by this template
        // and make their groups (by copying made groups and assinging slaveAddresses.
        for (auto const& slaveAddress : templateRegistered.second)
        {
            // Copy over the templates groups (deep copy, objects are recreated)
            std::vector<ModbusRegisterGroup> devicesGroups = templatesGroups;

            for (auto& devicesGroup : devicesGroups)
            {
                // Set slave address to the devices group.
                devicesGroup.setSlaveAddress(slaveAddress);

                // Create the watcher for all mappings inside of the group
                for (auto& mapping : devicesGroup.getRegisters())
                {
                    m_registerWatcherByReference.insert(std::pair<std::string, ModbusRegisterWatcher>(
                      mapping.getReference(), ModbusRegisterWatcher(slaveAddress, mapping)));
                }
            }

            m_deviceKeyBySlaveAddress.insert(
              std::pair<int, std::string>(slaveAddress, devices[slaveAddress]->getKey()));
            // Place the device slave address in memory with its ModbusRegisterGroups
            m_registerGroupsBySlaveAddress.insert(
              std::pair<int, std::vector<ModbusRegisterGroup>>(slaveAddress, devicesGroups));
            m_devicesStatusBySlaveAddress.insert(
              std::pair<int, DeviceStatus::Status>(slaveAddress, DeviceStatus::Status::OFFLINE));
        }
    }
}

ModbusBridge::~ModbusBridge()
{
    m_modbusClient.disconnect();
    stop();
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

void ModbusBridge::setOnConfigurationChange(const std::function<void(const std::string&, void*)>& onConfigurationChange)
{
    m_onConfigurationChange = onConfigurationChange;
}

void ModbusBridge::setOnDeviceStatusChange(
  const std::function<void(const std::string&, wolkabout::DeviceStatus::Status)>& onDeviceStatusChange)
{
    m_onDeviceStatusChange = onDeviceStatusChange;
}

// handleActuation and helper methods
// void handleActuatorForHoldingRegister() - handles single-value numeric actuations
void ModbusBridge::handleActuationForHoldingRegister(const wolkabout::ModbusRegisterMapping& modbusRegisterMapping,
                                                     ModbusRegisterWatcher& modbusRegisterWatcher,
                                                     const std::string& value)
{
    if (modbusRegisterMapping.getDataType() == ModbusRegisterMapping::DataType::INT16)
    {
        signed short typedValue = static_cast<signed short>(std::atol(value.c_str()));
        if (!m_modbusClient.writeHoldingRegister(modbusRegisterMapping.getSlaveAddress(),
                                                 modbusRegisterMapping.getAddress(), typedValue))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << modbusRegisterMapping.getAddress() << " Value: " << value;
            modbusRegisterWatcher.setValid(false);
        }
    }
    else if (modbusRegisterMapping.getDataType() == ModbusRegisterMapping::DataType::UINT16)
    {
        unsigned short typedValue = static_cast<unsigned short>(std::stoul(value.c_str()));
        if (!m_modbusClient.writeHoldingRegister(modbusRegisterMapping.getSlaveAddress(),
                                                 modbusRegisterMapping.getAddress(), typedValue))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << modbusRegisterMapping.getAddress() << " Value: " << value;
            modbusRegisterWatcher.setValid(false);
        }
    }
    else if (modbusRegisterMapping.getDataType() == ModbusRegisterMapping::DataType::REAL32)
    {
        float typedValue = std::stof(value.c_str());
        if (!m_modbusClient.writeHoldingRegister(modbusRegisterMapping.getSlaveAddress(),
                                                 modbusRegisterMapping.getAddress(), typedValue))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << modbusRegisterMapping.getAddress() << " Value: " << value;
            modbusRegisterWatcher.setValid(false);
        }
    }
}
// void handleActuatorForCoil() - handles single-value boolean actuations
void ModbusBridge::handleActuationForCoil(const wolkabout::ModbusRegisterMapping& modbusRegisterMapping,
                                          ModbusRegisterWatcher& modbusRegisterWatcher, const std::string& value)
{
    if (!m_modbusClient.writeCoil(modbusRegisterMapping.getSlaveAddress(), modbusRegisterMapping.getAddress(),
                                  value == "true"))
    {
        LOG(ERROR) << "ModbusBridge: Unable to write coil value - Register address: "
                   << modbusRegisterMapping.getAddress() << " Value: " << value;
        modbusRegisterWatcher.setValid(false);
    }
}
// void handleActuator() - decides on the handler for incoming actuation
void ModbusBridge::handleActuation(const std::string& deviceKey, const std::string& reference, const std::string& value)
{
    // Find the device if it exists, find the reference in his data, and change the value.
    LOG(INFO) << "ModbusBridge: Handling actuation on " << deviceKey << " reference '" << reference << "' - Value: '"
              << value << "'";

    if (m_slaveAddressesByDeviceKey.find(deviceKey) == m_slaveAddressesByDeviceKey.cend())
    {
        LOG(ERROR) << "ModbusBridge: No device with key '" << deviceKey << "'";
        return;
    }

    // After we found the device, check for the reference existence, and then the type of reference. Pass on to handler.
    auto const slaveAddress = m_slaveAddressesByDeviceKey[deviceKey];

    // TODO Fill in requested steps
}

// handleConfiguration and helper methods
// handleConfigurationForHoldingRegister() - handles single-value numeric configurations
void ModbusBridge::handleConfigurationForHoldingRegister(const ModbusRegisterMapping& modbusRegisterMapping,
                                                         ModbusRegisterWatcher& modbusRegisterWatcher,
                                                         const std::string& value)
{
    if (modbusRegisterMapping.getDataType() == ModbusRegisterMapping::DataType::INT16)
    {
        short shortValue = static_cast<short>(atoi(value.c_str()));
        if (!m_modbusClient.writeHoldingRegister(modbusRegisterMapping.getSlaveAddress(),
                                                 modbusRegisterMapping.getAddress(), shortValue))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << modbusRegisterMapping.getAddress() << " Value: " << shortValue;
            modbusRegisterWatcher.setValid(false);
        }
    }
    else if (modbusRegisterMapping.getDataType() == ModbusRegisterMapping::DataType::UINT16)
    {
        unsigned short uShortValue = static_cast<unsigned short>(atoi(value.c_str()));
        if (!m_modbusClient.writeHoldingRegister(modbusRegisterMapping.getSlaveAddress(),
                                                 modbusRegisterMapping.getAddress(), uShortValue))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << modbusRegisterMapping.getAddress() << " Value: " << uShortValue;
            modbusRegisterWatcher.setValid(false);
        }
    }
    else if (modbusRegisterMapping.getDataType() == ModbusRegisterMapping::DataType::REAL32)
    {
        float floatValue = atof(value.c_str());
        if (!m_modbusClient.writeHoldingRegister(modbusRegisterMapping.getSlaveAddress(),
                                                 modbusRegisterMapping.getAddress(), floatValue))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << modbusRegisterMapping.getAddress() << " Value: " << floatValue;
            modbusRegisterWatcher.setValid(false);
        }
    }
}
// handleConfigurationForHoldingRegisters() - handles multi-value numeric configurations
void ModbusBridge::handleConfigurationForHoldingRegisters(const ModbusRegisterMapping& modbusRegisterMapping,
                                                          ModbusRegisterWatcher& modbusRegisterWatcher,
                                                          std::vector<std::string> value)
{
    if (modbusRegisterMapping.getDataType() == ModbusRegisterMapping::DataType::INT16)
    {
        std::vector<short> shorts;
        shorts.reserve(value.size());
        for (const std::string& shortString : value)
        {
            shorts.push_back(static_cast<short>(atoi(shortString.c_str())));
        }
        if (!m_modbusClient.writeHoldingRegisters(modbusRegisterMapping.getSlaveAddress(),
                                                  modbusRegisterMapping.getAddress(), shorts))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << modbusRegisterMapping.getAddress();
            modbusRegisterWatcher.setValid(false);
        }
    }
    else if (modbusRegisterMapping.getDataType() == ModbusRegisterMapping::DataType::UINT16)
    {
        std::vector<unsigned short> uShorts;
        uShorts.reserve(value.size());
        for (const std::string& shortString : value)
        {
            uShorts.push_back(static_cast<unsigned short>(atoi(shortString.c_str())));
        }
        if (!m_modbusClient.writeHoldingRegisters(modbusRegisterMapping.getSlaveAddress(),
                                                  modbusRegisterMapping.getAddress(), uShorts))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << modbusRegisterMapping.getAddress();
            modbusRegisterWatcher.setValid(false);
        }
    }
    else if (modbusRegisterMapping.getDataType() == ModbusRegisterMapping::DataType::REAL32)
    {
        std::vector<float> floats;
        floats.reserve(value.size());
        for (const std::string& shortString : value)
        {
            floats.push_back(static_cast<float>(atof(shortString.c_str())));
        }
        if (!m_modbusClient.writeHoldingRegisters(modbusRegisterMapping.getSlaveAddress(),
                                                  modbusRegisterMapping.getAddress(), floats))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << modbusRegisterMapping.getAddress();
            modbusRegisterWatcher.setValid(false);
        }
    }
}
// handleConfigurationForCoil() - handles single-value boolean configurations
void ModbusBridge::handleConfigurationForCoil(const ModbusRegisterMapping& modbusRegisterMapping,
                                              ModbusRegisterWatcher& modbusRegisterWatcher, const std::string& value)
{
    std::vector<std::string> trueValues = {"true", "1", "1.0", "ON"};
    bool boolValue = std::find(trueValues.begin(), trueValues.end(), value) != trueValues.end();
    if (!m_modbusClient.writeCoil(modbusRegisterMapping.getSlaveAddress(), modbusRegisterMapping.getAddress(),
                                  boolValue))
    {
        LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                   << modbusRegisterMapping.getAddress() << " Value: " << boolValue;
        modbusRegisterWatcher.setValid(false);
    }
}
// handleConfiguration() - decides on the handler to handle incoming data
void ModbusBridge::handleConfiguration(const std::string& deviceKey,
                                       const std::vector<ConfigurationItem>& configuration)
{
    for (auto& config : configuration)
    {
        const std::string& reference = config.getReference();
        const ModbusRegisterMapping& modbusRegisterMapping =
          m_referenceToConfigurationModbusRegisterMapping.at(reference);
        ModbusRegisterWatcher& modbusRegisterWatcher = m_referenceToModbusRegisterWatcherMapping.at(reference);

        if (modbusRegisterMapping.getRegisterType() == ModbusRegisterMapping::RegisterType::COIL)
        {
            handleConfigurationForCoil(modbusRegisterMapping, modbusRegisterWatcher, config.getValues()[0]);
        }
        else
        {
            if (!modbusRegisterMapping.getLabelsAndAddresses().empty())
            {
                handleConfigurationForHoldingRegisters(modbusRegisterMapping, modbusRegisterWatcher,
                                                       config.getValues());
            }
            else
            {
                handleConfigurationForHoldingRegister(modbusRegisterMapping, modbusRegisterWatcher,
                                                      config.getValues()[0]);
            }
        }
    }
}

// getActuatorStatus and helper methods
// getActuatorStatusFromHoldingRegister() - handles status read for single-value numeric actuators
wolkabout::ActuatorStatus ModbusBridge::getActuatorStatusFromHoldingRegister(
  const wolkabout::ModbusRegisterMapping& modbusRegisterMapping, ModbusRegisterWatcher& modbusRegisterWatcher)
{
    if (modbusRegisterMapping.getDataType() == ModbusRegisterMapping::DataType::INT16)
    {
        signed short value;
        if (!m_modbusClient.readHoldingRegister(modbusRegisterMapping.getSlaveAddress(),
                                                modbusRegisterMapping.getAddress(), value))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read holding register with address '"
                       << modbusRegisterMapping.getAddress() << "'";

            modbusRegisterWatcher.setValid(false);
            return ActuatorStatus("", ActuatorStatus::State::ERROR);
        }

        modbusRegisterWatcher.update(value);

        return ActuatorStatus(std::to_string(value), ActuatorStatus::State::READY);
    }
    else if (modbusRegisterMapping.getDataType() == ModbusRegisterMapping::DataType::UINT16)
    {
        unsigned short value;
        if (!m_modbusClient.readHoldingRegister(modbusRegisterMapping.getSlaveAddress(),
                                                modbusRegisterMapping.getAddress(), value))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read holding register with address '"
                       << modbusRegisterMapping.getAddress() << "'";

            modbusRegisterWatcher.setValid(false);
            return ActuatorStatus("", ActuatorStatus::State::ERROR);
        }

        modbusRegisterWatcher.update(value);

        return ActuatorStatus(std::to_string(value), ActuatorStatus::State::READY);
    }
    else if (modbusRegisterMapping.getDataType() == ModbusRegisterMapping::DataType::REAL32)
    {
        float value;
        if (!m_modbusClient.readHoldingRegister(modbusRegisterMapping.getSlaveAddress(),
                                                modbusRegisterMapping.getAddress(), value))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read holding register with address '"
                       << modbusRegisterMapping.getAddress() << "'";

            modbusRegisterWatcher.setValid(false);
            return ActuatorStatus("", ActuatorStatus::State::ERROR);
        }

        modbusRegisterWatcher.update(value);

        return ActuatorStatus(std::to_string(value), ActuatorStatus::State::READY);
    }

    LOG(ERROR) << "ModbusBridge: Could not obtain actuator status from holding register - Unsupported data type";
    return ActuatorStatus("", ActuatorStatus::State::ERROR);
}
// getActuatorStatusFromCoil() - handles status read for single-value boolean actuators
wolkabout::ActuatorStatus ModbusBridge::getActuatorStatusFromCoil(
  const wolkabout::ModbusRegisterMapping& modbusRegisterMapping, ModbusRegisterWatcher& modbusRegisterWatcher)
{
    bool value;
    if (!m_modbusClient.readCoil(modbusRegisterMapping.getSlaveAddress(), modbusRegisterMapping.getAddress(), value))
    {
        LOG(ERROR) << "ModbusBridge: Unable to read coil with address '" << modbusRegisterMapping.getAddress() << "'";

        modbusRegisterWatcher.setValid(false);
        return ActuatorStatus("", ActuatorStatus::State::ERROR);
    }

    modbusRegisterWatcher.update(value);

    return ActuatorStatus(value ? "true" : "false", ActuatorStatus::State::READY);
}
// getActuatorStatus() - decides on the handler to handle the status request
wolkabout::ActuatorStatus ModbusBridge::getActuatorStatus(const std::string& /* deviceKey */,
                                                          const std::string& reference)
{
    LOG(INFO) << "ModbusBridge: Getting actuator status for reference '" << reference << "'";
    if (m_referenceToModbusRegisterMapping.find(reference) == m_referenceToModbusRegisterMapping.cend())
    {
        LOG(ERROR) << "ModbusBridge: No modbus register mapped to reference '" << reference << "'";
        return ActuatorStatus("", ActuatorStatus::State::ERROR);
    }

    if (m_referenceToModbusRegisterWatcherMapping.find(reference) == m_referenceToModbusRegisterWatcherMapping.cend())
    {
        m_referenceToModbusRegisterWatcherMapping.emplace(reference, ModbusRegisterWatcher{});
    }

    const ModbusRegisterMapping& modbusRegisterMapping = m_referenceToModbusRegisterMapping.at(reference);
    ModbusRegisterWatcher& modbusRegisterWatcher = m_referenceToModbusRegisterWatcherMapping.at(reference);

    if (modbusRegisterMapping.getRegisterType() != ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR &&
        modbusRegisterMapping.getRegisterType() != ModbusRegisterMapping::RegisterType::COIL)
    {
        LOG(ERROR)
          << "ModbusBridge: Modbus register mapped to reference '" << reference
          << "' can not be treated as actuator - Modbus register must be of type 'HOLDING_REGISTER_ACTUATOR' or 'COIL'";
        return ActuatorStatus("", ActuatorStatus::State::ERROR);
    }

    return modbusRegisterMapping.getRegisterType() == ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR ?
             getActuatorStatusFromHoldingRegister(modbusRegisterMapping, modbusRegisterWatcher) :
             getActuatorStatusFromCoil(modbusRegisterMapping, modbusRegisterWatcher);
}

// getConfiguration and helper methods
// getConfigurationStatusFromHoldingRegister() - handles status read for any-value-count numeric configurations
ConfigurationItem ModbusBridge::getConfigurationStatusFromHoldingRegister(
  const ModbusRegisterMapping& modbusRegisterMapping, ModbusRegisterWatcher& modbusRegisterWatcher)
{
    std::vector<std::string> configurationValues;
    if (modbusRegisterMapping.getDataType() == ModbusRegisterMapping::DataType::INT16)
    {
        if (!modbusRegisterMapping.getLabelsAndAddresses().empty())
        {
            std::vector<short> values;
            if (!m_modbusClient.readHoldingRegisters(
                  modbusRegisterMapping.getSlaveAddress(), modbusRegisterMapping.getAddress(),
                  static_cast<int>(modbusRegisterMapping.getLabelsAndAddresses().size()), values))
            {
                LOG(ERROR) << "ModbusBridge: Unable to read holding registers from address '"
                           << modbusRegisterMapping.getAddress() << "'";

                modbusRegisterWatcher.setValid(false);
                return ConfigurationItem(std::vector<std::string>(), "");
            }
            modbusRegisterWatcher.update(values);
        }
        else
        {
            short value;
            if (!m_modbusClient.readHoldingRegister(modbusRegisterMapping.getSlaveAddress(),
                                                    modbusRegisterMapping.getAddress(), value))
            {
                LOG(ERROR) << "ModbusBridge: Unable to read holding registers from address '"
                           << modbusRegisterMapping.getAddress() << "'";

                modbusRegisterWatcher.setValid(false);
                return ConfigurationItem(std::vector<std::string>(), "");
            }
            modbusRegisterWatcher.update(value);
        }
    }
    else if (modbusRegisterMapping.getDataType() == ModbusRegisterMapping::DataType::UINT16)
    {
        if (!modbusRegisterMapping.getLabelsAndAddresses().empty())
        {
            std::vector<unsigned short> values;
            if (!m_modbusClient.readHoldingRegisters(
                  modbusRegisterMapping.getSlaveAddress(), modbusRegisterMapping.getAddress(),
                  static_cast<int>(modbusRegisterMapping.getLabelsAndAddresses().size()), values))
            {
                LOG(ERROR) << "ModbusBridge: Unable to read holding registers from address '"
                           << modbusRegisterMapping.getAddress() << "'";

                modbusRegisterWatcher.setValid(false);
                return ConfigurationItem(std::vector<std::string>(), "");
            }
            modbusRegisterWatcher.update(values);
        }
        else
        {
            unsigned short value;
            if (!m_modbusClient.readHoldingRegister(modbusRegisterMapping.getSlaveAddress(),
                                                    modbusRegisterMapping.getAddress(), value))
            {
                LOG(ERROR) << "ModbusBridge: Unable to read holding registers from address '"
                           << modbusRegisterMapping.getAddress() << "'";

                modbusRegisterWatcher.setValid(false);
                return ConfigurationItem(std::vector<std::string>(), "");
            }
            modbusRegisterWatcher.update(value);
        }
    }
    else if (modbusRegisterMapping.getDataType() == ModbusRegisterMapping::DataType::REAL32)
    {
        if (!modbusRegisterMapping.getLabelsAndAddresses().empty())
        {
            std::vector<float> values;
            if (!m_modbusClient.readHoldingRegisters(
                  modbusRegisterMapping.getSlaveAddress(), modbusRegisterMapping.getAddress(),
                  static_cast<int>(modbusRegisterMapping.getLabelsAndAddresses().size()), values))
            {
                LOG(ERROR) << "ModbusBridge: Unable to read holding registers from address '"
                           << modbusRegisterMapping.getAddress() << "'";

                modbusRegisterWatcher.setValid(false);
                return ConfigurationItem(std::vector<std::string>(), "");
            }
            modbusRegisterWatcher.update(values);
        }
        else
        {
            float value;
            if (!m_modbusClient.readHoldingRegister(modbusRegisterMapping.getSlaveAddress(),
                                                    modbusRegisterMapping.getAddress(), value))
            {
                LOG(ERROR) << "ModbusBridge: Unable to read holding registers from address '"
                           << modbusRegisterMapping.getAddress() << "'";

                modbusRegisterWatcher.setValid(false);
                return ConfigurationItem(std::vector<std::string>(), "");
            }
            modbusRegisterWatcher.update(value);
        }
    }

    auto values = StringUtils::tokenize(modbusRegisterWatcher.getValue(), ",");
    for (auto& value : values)
    {
        configurationValues.emplace_back(value);
    }

    return ConfigurationItem(configurationValues, modbusRegisterMapping.getReference());
}
// getConfigurationStatusFromCoil() - handles status read for single-value boolean configurations
ConfigurationItem ModbusBridge::getConfigurationStatusFromCoil(const ModbusRegisterMapping& modbusRegisterMapping,
                                                               ModbusRegisterWatcher& modbusRegisterWatcher)
{
    bool value;
    if (!m_modbusClient.readCoil(modbusRegisterMapping.getSlaveAddress(), modbusRegisterMapping.getAddress(), value))
    {
        LOG(ERROR) << "ModbusBridge: Unable to read coil on address '" << modbusRegisterMapping.getAddress() << "'";

        modbusRegisterWatcher.setValid(false);
        return ConfigurationItem(std::vector<std::string>(), "");
    }
    modbusRegisterWatcher.update(value);
    return ConfigurationItem({modbusRegisterWatcher.getValue()}, modbusRegisterMapping.getReference());
}
// getConfiguration() - decides on the handler to handle the status request
std::vector<ConfigurationItem> ModbusBridge::getConfiguration(const std::string& deviceKey)
{
    std::vector<ConfigurationItem> configurations;
    for (auto& mapping : m_referenceToConfigurationModbusRegisterMapping)
    {
        if (m_referenceToModbusRegisterWatcherMapping.find(mapping.first) ==
            m_referenceToModbusRegisterWatcherMapping.cend())
        {
            m_referenceToModbusRegisterWatcherMapping.emplace(mapping.first, ModbusRegisterWatcher{});
        }

        ModbusRegisterWatcher& watcher = m_referenceToModbusRegisterWatcherMapping.at(mapping.first);

        switch (mapping.second.getRegisterType())
        {
        case ModbusRegisterMapping::RegisterType::COIL:
            configurations.emplace_back(getConfigurationStatusFromCoil(mapping.second, watcher));
            break;
        case ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR:
            configurations.emplace_back(getConfigurationStatusFromHoldingRegister(mapping.second, watcher));
            break;
        }
    }

    return configurations;
}

// getDeviceStatus() - handles status request about a specific device
wolkabout::DeviceStatus::Status ModbusBridge::getDeviceStatus(const std::string& deviceKey)
{
    return m_modbusClient.isConnected() ? DeviceStatus::Status::CONNECTED : DeviceStatus::Status::OFFLINE;
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
            m_modbusClient.disconnect();
            if (m_onDeviceStatusChange)
            {
                m_onDeviceStatusChange(wolkabout::DeviceStatus::Status::OFFLINE);
            }

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
                m_onDeviceStatusChange(wolkabout::DeviceStatus::Status::CONNECTED);
            }
            readAndReportModbusRegistersValues();
        }

        std::this_thread::sleep_for(m_registerReadPeriod);
    }
}

// readAndReport - loop for reading all devices and their groups
// with helper methods, for reading specific type of group

void ModbusBridge::readHoldingRegisters(ModbusRegisterGroup& group)
{
    // TODO implement workings of method
}

void ModbusBridge::readInputRegisters(ModbusRegisterGroup& group)
{
    // TODO implement workings of method
}

void ModbusBridge::readCoils(ModbusRegisterGroup& group)
{
    // TODO implement workings of method
}

void ModbusBridge::readDiscreteInputs(ModbusRegisterGroup& group)
{
    // TODO implement workings of method
}

void ModbusBridge::readAndReportModbusRegistersValues()
{
    LOG(DEBUG) << "ModbusBridge: Reading and reporting register values";

    std::map<int, bool> slavesRead;

    for (ModbusRegisterGroup& modbusRegisterGroup : m_modbusRegisterGroups)
    {
        switch (modbusRegisterGroup.getRegisterType())
        {
        case ModbusRegisterMapping::RegisterType::COIL:
        {
            std::vector<bool> values;
            if (!m_modbusClient.readCoils(modbusRegisterGroup.getSlaveAddress(),
                                          modbusRegisterGroup.getStartingRegisterAddress(),
                                          modbusRegisterGroup.getRegisterCount(), values))
            {
                LOG(ERROR) << "ModbusBridge: Unable to read coils on slave " << modbusRegisterGroup.getSlaveAddress()
                           << " from address '" << modbusRegisterGroup.getStartingRegisterAddress() << "'";

                if (slavesRead.count(modbusRegisterGroup.getSlaveAddress()) == 0)
                {
                    slavesRead.insert(std::make_pair(modbusRegisterGroup.getSlaveAddress(), false));
                }
                continue;
            }

            slavesRead[modbusRegisterGroup.getSlaveAddress()] = true;

            for (int i = 0; i < modbusRegisterGroup.getMappingsCount(); ++i)
            {
                bool value = values[i];
                const ModbusRegisterMapping& modbusRegisterMapping = modbusRegisterGroup.getRegisters()[i];
                ModbusRegisterWatcher& modbusRegisterWatcher =
                  m_referenceToModbusRegisterWatcherMapping.at(modbusRegisterMapping.getReference());
                if (modbusRegisterWatcher.update(value))
                {
                    LOG(INFO) << "ModbusBridge: Actuator value changed - Reference: '"
                              << modbusRegisterMapping.getReference() << "' Value: '"
                              << modbusRegisterWatcher.getValue() << "'";

                    switch (modbusRegisterMapping.getMappingType())
                    {
                    case ModbusRegisterMapping::MappingType::DEFAULT:
                    case ModbusRegisterMapping::MappingType::ACTUATOR:
                        if (m_onActuatorStatusChange)
                        {
                            m_onActuatorStatusChange(modbusRegisterMapping.getReference());
                        }
                        break;
                    case ModbusRegisterMapping::MappingType::CONFIGURATION:
                        if (m_onConfigurationChange)
                        {
                            m_onConfigurationChange();
                        }
                        break;
                    default:
                        LOG(ERROR) << "Mapping " << modbusRegisterMapping.getName()
                                   << " (address: " << modbusRegisterMapping.getAddress() << ") "
                                   << "can't be that mapping type!";
                        break;
                    }
                }
            }
            break;
        }

        case ModbusRegisterMapping::RegisterType::INPUT_CONTACT:
        {
            std::vector<bool> values;
            if (!m_modbusClient.readInputContacts(modbusRegisterGroup.getSlaveAddress(),
                                                  modbusRegisterGroup.getStartingRegisterAddress(),
                                                  modbusRegisterGroup.getRegisterCount(), values))
            {
                LOG(ERROR) << "ModbusBridge: Unable to read input contacts on slave "
                           << modbusRegisterGroup.getSlaveAddress() << " from address '"
                           << modbusRegisterGroup.getStartingRegisterAddress() << "'";
                if (slavesRead.count(modbusRegisterGroup.getSlaveAddress()) == 0)
                {
                    slavesRead.insert(std::make_pair(modbusRegisterGroup.getSlaveAddress(), false));
                }
                continue;
            }

            slavesRead[modbusRegisterGroup.getSlaveAddress()] = true;

            for (int i = 0; i < modbusRegisterGroup.getMappingsCount(); ++i)
            {
                bool value = values[i];
                const ModbusRegisterMapping& modbusRegisterMapping = modbusRegisterGroup.getRegisters()[i];
                ModbusRegisterWatcher& modbusRegisterWatcher =
                  m_referenceToModbusRegisterWatcherMapping.at(modbusRegisterMapping.getReference());
                if (modbusRegisterWatcher.update(value))
                {
                    LOG(INFO) << "ModbusBridge: Sensor value - Reference: '" << modbusRegisterMapping.getReference()
                              << "' Value: '" << modbusRegisterWatcher.getValue() << "'";

                    switch (modbusRegisterMapping.getMappingType())
                    {
                    case ModbusRegisterMapping::MappingType::DEFAULT:
                    case ModbusRegisterMapping::MappingType::SENSOR:
                        if (m_onSensorReading)
                        {
                            m_onSensorReading(modbusRegisterMapping.getReference(), modbusRegisterWatcher.getValue());
                        }
                        break;
                    case ModbusRegisterMapping::MappingType::ALARM:
                        if (m_onAlarmChange)
                        {
                            m_onAlarmChange(modbusRegisterMapping.getReference(), value);
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
        case ModbusRegisterMapping::RegisterType::INPUT_REGISTER:
        {
            switch (modbusRegisterGroup.getDataType())
            {
            case ModbusRegisterMapping::DataType::INT16:
            {
                std::vector<signed short> values;
                if (!m_modbusClient.readInputRegisters(modbusRegisterGroup.getSlaveAddress(),
                                                       modbusRegisterGroup.getStartingRegisterAddress(),
                                                       modbusRegisterGroup.getRegisterCount(), values))
                {
                    LOG(ERROR) << "ModbusBridge: Unable to read input registers on slave "
                               << modbusRegisterGroup.getSlaveAddress() << " from address '"
                               << modbusRegisterGroup.getStartingRegisterAddress() << "'";
                    if (slavesRead.count(modbusRegisterGroup.getSlaveAddress()) == 0)
                    {
                        slavesRead.insert(std::make_pair(modbusRegisterGroup.getSlaveAddress(), false));
                    }
                    continue;
                }

                slavesRead[modbusRegisterGroup.getSlaveAddress()] = true;

                for (int i = 0; i < modbusRegisterGroup.getMappingsCount(); ++i)
                {
                    signed short value = values[i];
                    const ModbusRegisterMapping& modbusRegisterMapping = modbusRegisterGroup.getRegisters()[i];
                    ModbusRegisterWatcher& modbusRegisterWatcher =
                      m_referenceToModbusRegisterWatcherMapping.at(modbusRegisterMapping.getReference());
                    if (modbusRegisterWatcher.update(value))
                    {
                        LOG(INFO) << "ModbusBridge: Sensor value - Reference: '" << modbusRegisterMapping.getReference()
                                  << "' Value: '" << modbusRegisterWatcher.getValue() << "'";

                        // This here isn't edited because the input_register can't be anything else than a sensor!
                        if (m_onSensorReading)
                        {
                            m_onSensorReading(modbusRegisterMapping.getReference(), modbusRegisterWatcher.getValue());
                        }
                    }
                }
                break;
            }
            case ModbusRegisterMapping::DataType::UINT16:
            {
                std::vector<unsigned short> values;
                if (!m_modbusClient.readInputRegisters(modbusRegisterGroup.getSlaveAddress(),
                                                       modbusRegisterGroup.getStartingRegisterAddress(),
                                                       modbusRegisterGroup.getRegisterCount(), values))
                {
                    LOG(ERROR) << "ModbusBridge: Unable to read input registers on slave "
                               << modbusRegisterGroup.getSlaveAddress() << " from address '"
                               << modbusRegisterGroup.getStartingRegisterAddress() << "'";
                    if (slavesRead.count(modbusRegisterGroup.getSlaveAddress()) == 0)
                    {
                        slavesRead.insert(std::make_pair(modbusRegisterGroup.getSlaveAddress(), false));
                    }
                    continue;
                }

                slavesRead[modbusRegisterGroup.getSlaveAddress()] = true;

                for (int i = 0; i < modbusRegisterGroup.getMappingsCount(); ++i)
                {
                    unsigned short value = values[i];
                    const ModbusRegisterMapping& modbusRegisterMapping = modbusRegisterGroup.getRegisters()[i];
                    ModbusRegisterWatcher& modbusRegisterWatcher =
                      m_referenceToModbusRegisterWatcherMapping.at(modbusRegisterMapping.getReference());
                    if (modbusRegisterWatcher.update(value))
                    {
                        LOG(INFO) << "ModbusBridge: Sensor value - Reference: '" << modbusRegisterMapping.getReference()
                                  << "' Value: '" << modbusRegisterWatcher.getValue() << "'";

                        // This here isn't edited because the input_register can't be anything else than a sensor!
                        if (m_onSensorReading)
                        {
                            m_onSensorReading(modbusRegisterMapping.getReference(), modbusRegisterWatcher.getValue());
                        }
                    }
                }
                break;
            }
            case ModbusRegisterMapping::DataType::REAL32:
            {
                std::vector<float> values;
                if (!m_modbusClient.readInputRegisters(modbusRegisterGroup.getSlaveAddress(),
                                                       modbusRegisterGroup.getStartingRegisterAddress(),
                                                       modbusRegisterGroup.getRegisterCount(), values))
                {
                    LOG(ERROR) << "ModbusBridge: Unable to read input registers on slave "
                               << modbusRegisterGroup.getSlaveAddress() << " from address '"
                               << modbusRegisterGroup.getStartingRegisterAddress() << "'";
                    if (slavesRead.count(modbusRegisterGroup.getSlaveAddress()) == 0)
                    {
                        slavesRead.insert(std::make_pair(modbusRegisterGroup.getSlaveAddress(), false));
                    }
                    continue;
                }

                slavesRead[modbusRegisterGroup.getSlaveAddress()] = true;

                for (int i = 0; i < modbusRegisterGroup.getMappingsCount(); ++i)
                {
                    float value = values[i];
                    const ModbusRegisterMapping& modbusRegisterMapping = modbusRegisterGroup.getRegisters()[i];
                    ModbusRegisterWatcher& modbusRegisterWatcher =
                      m_referenceToModbusRegisterWatcherMapping.at(modbusRegisterMapping.getReference());
                    if (modbusRegisterWatcher.update(value))
                    {
                        LOG(INFO) << "ModbusBridge: Sensor value - Reference: '" << modbusRegisterMapping.getReference()
                                  << "' Value: '" << modbusRegisterWatcher.getValue() << "'";

                        // This here isn't edited because the input_register can't be anything else than a sensor!
                        if (m_onSensorReading)
                        {
                            m_onSensorReading(modbusRegisterMapping.getReference(), modbusRegisterWatcher.getValue());
                        }
                    }
                }
                break;
            }
            }
            break;
        }
        case ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_SENSOR:
        {
            switch (modbusRegisterGroup.getDataType())
            {
            case ModbusRegisterMapping::DataType::INT16:
            {
                std::vector<signed short> values;
                if (!m_modbusClient.readHoldingRegisters(modbusRegisterGroup.getSlaveAddress(),
                                                         modbusRegisterGroup.getStartingRegisterAddress(),
                                                         modbusRegisterGroup.getRegisterCount(), values))
                {
                    LOG(ERROR) << "ModbusBridge: Unable to read holding registers on slave "
                               << modbusRegisterGroup.getSlaveAddress() << " from address '"
                               << modbusRegisterGroup.getStartingRegisterAddress() << "'";
                    if (slavesRead.count(modbusRegisterGroup.getSlaveAddress()) == 0)
                    {
                        slavesRead.insert(std::make_pair(modbusRegisterGroup.getSlaveAddress(), false));
                    }
                    continue;
                }

                slavesRead[modbusRegisterGroup.getSlaveAddress()] = true;

                for (int i = 0; i < modbusRegisterGroup.getMappingsCount(); ++i)
                {
                    signed short value = values[i];
                    const ModbusRegisterMapping& modbusRegisterMapping = modbusRegisterGroup.getRegisters()[i];
                    ModbusRegisterWatcher& modbusRegisterWatcher =
                      m_referenceToModbusRegisterWatcherMapping.at(modbusRegisterMapping.getReference());
                    if (modbusRegisterWatcher.update(value))
                    {
                        LOG(INFO) << "ModbusBridge: Sensor value - Reference: '" << modbusRegisterMapping.getReference()
                                  << "' Value: '" << modbusRegisterWatcher.getValue() << "'";

                        // This here isn't edited because the holding_register_sensor can't be anything else than a
                        // sensor!
                        if (m_onSensorReading)
                        {
                            m_onSensorReading(modbusRegisterMapping.getReference(), modbusRegisterWatcher.getValue());
                        }
                    }
                }
                break;
            }
            case ModbusRegisterMapping::DataType::UINT16:
            {
                std::vector<unsigned short> values;
                if (!m_modbusClient.readHoldingRegisters(modbusRegisterGroup.getSlaveAddress(),
                                                         modbusRegisterGroup.getStartingRegisterAddress(),
                                                         modbusRegisterGroup.getRegisterCount(), values))
                {
                    LOG(ERROR) << "ModbusBridge: Unable to read holding registers on slave "
                               << modbusRegisterGroup.getSlaveAddress() << " from address '"
                               << modbusRegisterGroup.getStartingRegisterAddress() << "'";
                    if (slavesRead.count(modbusRegisterGroup.getSlaveAddress()) == 0)
                    {
                        slavesRead.insert(std::make_pair(modbusRegisterGroup.getSlaveAddress(), false));
                    }
                    continue;
                }

                slavesRead[modbusRegisterGroup.getSlaveAddress()] = true;

                for (int i = 0; i < modbusRegisterGroup.getMappingsCount(); ++i)
                {
                    unsigned short value = values[i];
                    const ModbusRegisterMapping& modbusRegisterMapping = modbusRegisterGroup.getRegisters()[i];
                    ModbusRegisterWatcher& modbusRegisterWatcher =
                      m_referenceToModbusRegisterWatcherMapping.at(modbusRegisterMapping.getReference());
                    if (modbusRegisterWatcher.update(value))
                    {
                        LOG(INFO) << "ModbusBridge: Sensor value - Reference: '" << modbusRegisterMapping.getReference()
                                  << "' Value: '" << modbusRegisterWatcher.getValue() << "'";

                        // This here isn't edited because the holding_register_sensor can't be anything else than a
                        // sensor!
                        if (m_onSensorReading)
                        {
                            m_onSensorReading(modbusRegisterMapping.getReference(), modbusRegisterWatcher.getValue());
                        }
                    }
                }
                break;
            }
            case ModbusRegisterMapping::DataType::REAL32:
            {
                std::vector<float> values;
                if (!m_modbusClient.readHoldingRegisters(modbusRegisterGroup.getSlaveAddress(),
                                                         modbusRegisterGroup.getStartingRegisterAddress(),
                                                         modbusRegisterGroup.getRegisterCount(), values))
                {
                    LOG(ERROR) << "ModbusBridge: Unable to read holding registers on slave "
                               << modbusRegisterGroup.getSlaveAddress() << " from address '"
                               << modbusRegisterGroup.getStartingRegisterAddress() << "'";
                    if (slavesRead.count(modbusRegisterGroup.getSlaveAddress()) == 0)
                    {
                        slavesRead.insert(std::make_pair(modbusRegisterGroup.getSlaveAddress(), false));
                    }
                    continue;
                }

                slavesRead[modbusRegisterGroup.getSlaveAddress()] = true;

                for (int i = 0; i < modbusRegisterGroup.getMappingsCount(); ++i)
                {
                    float value = values[i];
                    const ModbusRegisterMapping& modbusRegisterMapping = modbusRegisterGroup.getRegisters()[i];
                    ModbusRegisterWatcher& modbusRegisterWatcher =
                      m_referenceToModbusRegisterWatcherMapping.at(modbusRegisterMapping.getReference());
                    if (modbusRegisterWatcher.update(value))
                    {
                        LOG(INFO) << "ModbusBridge: Sensor value - Reference: '" << modbusRegisterMapping.getReference()
                                  << "' Value: '" << modbusRegisterWatcher.getValue() << "'";

                        // This here isn't edited because the holding_register_sensor can't be anything else than a
                        // sensor!
                        if (m_onSensorReading)
                        {
                            m_onSensorReading(modbusRegisterMapping.getReference(), modbusRegisterWatcher.getValue());
                        }
                    }
                }
                break;
            }
            }
            break;
        }
        case ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR:
        {
            switch (modbusRegisterGroup.getDataType())
            {
            case ModbusRegisterMapping::DataType::INT16:
            {
                std::vector<signed short> values;
                if (!m_modbusClient.readHoldingRegisters(modbusRegisterGroup.getSlaveAddress(),
                                                         modbusRegisterGroup.getStartingRegisterAddress(),
                                                         modbusRegisterGroup.getRegisterCount(), values))
                {
                    LOG(ERROR) << "ModbusBridge: Unable to read holding registers on slave "
                               << modbusRegisterGroup.getSlaveAddress() << " from address '"
                               << modbusRegisterGroup.getStartingRegisterAddress() << "'";
                    if (slavesRead.count(modbusRegisterGroup.getSlaveAddress()) == 0)
                    {
                        slavesRead.insert(std::make_pair(modbusRegisterGroup.getSlaveAddress(), false));
                    }
                    continue;
                }

                slavesRead[modbusRegisterGroup.getSlaveAddress()] = true;

                for (int i = 0, j = 0; i < modbusRegisterGroup.getMappingsCount(); ++i)
                {
                    const ModbusRegisterMapping& modbusRegisterMapping = modbusRegisterGroup.getRegisters()[i];
                    ModbusRegisterWatcher& modbusRegisterWatcher =
                      m_referenceToModbusRegisterWatcherMapping.at(modbusRegisterMapping.getReference());

                    bool updated;
                    // separate the values of group for each mapping they need to go to
                    if (modbusRegisterMapping.getLabelsAndAddresses().empty())
                    {
                        signed short value = values[j++];
                        updated = modbusRegisterWatcher.update(value);
                    }
                    else
                    {
                        std::vector<short> groupValues;
                        for (int k = 0; k < modbusRegisterMapping.getLabelsAndAddresses().size(); k++)
                        {
                            groupValues.push_back(values[j++]);
                        }
                        updated = modbusRegisterWatcher.update(groupValues);
                    }

                    if (updated)
                    {
                        LOG(INFO) << "ModbusBridge: "
                                  << ((modbusRegisterMapping.getMappingType() ==
                                       ModbusRegisterMapping::MappingType::CONFIGURATION) ?
                                        "Configuration" :
                                        "Actuator")
                                  << " value changed - Reference: '" << modbusRegisterMapping.getReference()
                                  << "' Value: '" << modbusRegisterWatcher.getValue() << "'";

                        switch (modbusRegisterMapping.getMappingType())
                        {
                        case ModbusRegisterMapping::MappingType::DEFAULT:
                        case ModbusRegisterMapping::MappingType::ACTUATOR:
                            if (m_onActuatorStatusChange)
                            {
                                m_onActuatorStatusChange(modbusRegisterMapping.getReference());
                            }
                            break;
                        case ModbusRegisterMapping::MappingType::CONFIGURATION:
                            if (m_onConfigurationChange)
                            {
                                m_onConfigurationChange();
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
                if (!m_modbusClient.readHoldingRegisters(modbusRegisterGroup.getSlaveAddress(),
                                                         modbusRegisterGroup.getStartingRegisterAddress(),
                                                         modbusRegisterGroup.getRegisterCount(), values))
                {
                    LOG(ERROR) << "ModbusBridge: Unable to read holding registers on slave "
                               << modbusRegisterGroup.getSlaveAddress() << " from address '"
                               << modbusRegisterGroup.getStartingRegisterAddress() << "'";
                    if (slavesRead.count(modbusRegisterGroup.getSlaveAddress()) == 0)
                    {
                        slavesRead.insert(std::make_pair(modbusRegisterGroup.getSlaveAddress(), false));
                    }
                    continue;
                }

                slavesRead[modbusRegisterGroup.getSlaveAddress()] = true;

                for (int i = 0, j = 0; i < modbusRegisterGroup.getMappingsCount(); ++i)
                {
                    const ModbusRegisterMapping& modbusRegisterMapping = modbusRegisterGroup.getRegisters()[i];
                    ModbusRegisterWatcher& modbusRegisterWatcher =
                      m_referenceToModbusRegisterWatcherMapping.at(modbusRegisterMapping.getReference());

                    bool updated;
                    // separate the values of group for each mapping they need to go to
                    if (modbusRegisterMapping.getLabelsAndAddresses().empty())
                    {
                        unsigned short value = values[j++];
                        updated = modbusRegisterWatcher.update(value);
                    }
                    else
                    {
                        std::vector<unsigned short> groupValues;
                        for (int k = 0; k < modbusRegisterMapping.getLabelsAndAddresses().size(); k++)
                        {
                            groupValues.push_back(values[j++]);
                        }
                        updated = modbusRegisterWatcher.update(groupValues);
                    }
                    if (updated)
                    {
                        LOG(INFO) << "ModbusBridge: Actuator value changed - Reference: '"
                                  << modbusRegisterMapping.getReference() << "' Value: '"
                                  << modbusRegisterWatcher.getValue() << "'";

                        switch (modbusRegisterMapping.getMappingType())
                        {
                        case ModbusRegisterMapping::MappingType::DEFAULT:
                        case ModbusRegisterMapping::MappingType::ACTUATOR:
                            if (m_onActuatorStatusChange)
                            {
                                m_onActuatorStatusChange(modbusRegisterMapping.getReference());
                            }
                            break;
                        case ModbusRegisterMapping::MappingType::CONFIGURATION:
                            if (m_onConfigurationChange)
                            {
                                m_onConfigurationChange();
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
                if (!m_modbusClient.readHoldingRegisters(modbusRegisterGroup.getSlaveAddress(),
                                                         modbusRegisterGroup.getStartingRegisterAddress(),
                                                         modbusRegisterGroup.getRegisterCount(), values))
                {
                    LOG(ERROR) << "ModbusBridge: Unable to read holding registers on slave "
                               << modbusRegisterGroup.getSlaveAddress() << " from address '"
                               << modbusRegisterGroup.getStartingRegisterAddress() << "'";
                    if (slavesRead.count(modbusRegisterGroup.getSlaveAddress()) == 0)
                    {
                        slavesRead.insert(std::make_pair(modbusRegisterGroup.getSlaveAddress(), false));
                    }
                    continue;
                }

                slavesRead[modbusRegisterGroup.getSlaveAddress()] = true;

                for (int i = 0, j = 0; i < modbusRegisterGroup.getMappingsCount(); ++i)
                {
                    const ModbusRegisterMapping& modbusRegisterMapping = modbusRegisterGroup.getRegisters()[i];
                    ModbusRegisterWatcher& modbusRegisterWatcher =
                      m_referenceToModbusRegisterWatcherMapping.at(modbusRegisterMapping.getReference());

                    bool updated;
                    // separate the values of group for each mapping they need to go to
                    if (modbusRegisterMapping.getLabelsAndAddresses().empty())
                    {
                        float value = values[j++];
                        updated = modbusRegisterWatcher.update(value);
                    }
                    else
                    {
                        std::vector<float> groupValues;
                        for (int k = 0; k < modbusRegisterMapping.getLabelsAndAddresses().size(); k++)
                        {
                            groupValues.push_back(values[j++]);
                        }
                        updated = modbusRegisterWatcher.update(groupValues);
                    }
                    if (updated)
                    {
                        LOG(INFO) << "ModbusBridge: Actuator value changed - Reference: '"
                                  << modbusRegisterMapping.getReference() << "' Value: '"
                                  << modbusRegisterWatcher.getValue() << "'";

                        switch (modbusRegisterMapping.getMappingType())
                        {
                        case ModbusRegisterMapping::MappingType::DEFAULT:
                        case ModbusRegisterMapping::MappingType::ACTUATOR:
                            if (m_onActuatorStatusChange)
                            {
                                m_onActuatorStatusChange(modbusRegisterMapping.getReference());
                            }
                            break;
                        case ModbusRegisterMapping::MappingType::CONFIGURATION:
                            if (m_onConfigurationChange)
                            {
                                m_onConfigurationChange();
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
            }
            break;
        }
        }
    }

    if (slavesRead.size() == 1)
    {
        m_shouldReconnect = !slavesRead.begin()->second;
    }
}
}    // namespace wolkabout
