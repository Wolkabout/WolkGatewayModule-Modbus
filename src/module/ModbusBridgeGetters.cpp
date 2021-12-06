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
#include "core/utilities/Logger.h"
#include "more_modbus/mappings/BoolMapping.h"
#include "more_modbus/mappings/FloatMapping.h"
#include "more_modbus/mappings/Int16Mapping.h"
#include "more_modbus/mappings/Int32Mapping.h"
#include "more_modbus/mappings/StringMapping.h"
#include "more_modbus/mappings/UInt16Mapping.h"
#include "more_modbus/mappings/UInt32Mapping.h"

namespace wolkabout
{
std::string ModbusBridge::readFromMapping(const std::shared_ptr<RegisterMapping>& mapping)
{
    switch (mapping->getOutputType())
    {
    case RegisterMapping::OutputType::BOOL:
        return readFromBoolMapping(mapping);
    case RegisterMapping::OutputType::UINT16:
        return readFromUInt16Mapping(mapping);
    case RegisterMapping::OutputType::INT16:
        return readFromInt16Mapping(mapping);
    case RegisterMapping::OutputType::UINT32:
        return readFromUInt32Mapping(mapping);
    case RegisterMapping::OutputType::INT32:
        return readFromInt32Mapping(mapping);
    case RegisterMapping::OutputType::FLOAT:
        return readFromFloatMapping(mapping);
    case RegisterMapping::OutputType::STRING:
        return readFromStringMapping(mapping);
    }
    return "";
}

std::string ModbusBridge::readFromBoolMapping(const std::shared_ptr<RegisterMapping>& mapping)
{
    const auto& boolMapping = (const std::shared_ptr<BoolMapping>&)mapping;
    return std::to_string(boolMapping->getBoolValue());
}

std::string ModbusBridge::readFromUInt16Mapping(const std::shared_ptr<RegisterMapping>& mapping)
{
    const auto& uint16Mapping = (const std::shared_ptr<UInt16Mapping>&)mapping;
    return std::to_string(uint16Mapping->getUint16Value());
}

std::string ModbusBridge::readFromInt16Mapping(const std::shared_ptr<RegisterMapping>& mapping)
{
    const auto& int16Mapping = (const std::shared_ptr<Int16Mapping>&)mapping;
    return std::to_string(int16Mapping->getInt16Value());
}

std::string ModbusBridge::readFromUInt32Mapping(const std::shared_ptr<RegisterMapping>& mapping)
{
    const auto& uint32Mapping = (const std::shared_ptr<UInt32Mapping>&)mapping;
    return std::to_string(uint32Mapping->getUint32Value());
}

std::string ModbusBridge::readFromInt32Mapping(const std::shared_ptr<RegisterMapping>& mapping)
{
    const auto& int32Mapping = (const std::shared_ptr<Int32Mapping>&)mapping;
    return std::to_string(int32Mapping->getInt32Value());
}

std::string ModbusBridge::readFromFloatMapping(const std::shared_ptr<RegisterMapping>& mapping)
{
    const auto& floatMapping = (const std::shared_ptr<FloatMapping>&)mapping;
    return std::to_string(floatMapping->getFloatValue());
}

std::string ModbusBridge::readFromStringMapping(const std::shared_ptr<RegisterMapping>& mapping)
{
    const auto& stringMapping = (const std::shared_ptr<StringMapping>&)mapping;
    return stringMapping->getStringValue();
}

// getActuatorStatus and helper methods
// getActuatorStatusFromHoldingRegister() - handles
wolkabout::ActuatorStatus ModbusBridge::getActuatorStatusFromHoldingRegister(
  const std::shared_ptr<RegisterMapping>& mapping)
{
    if (!mapping->isValid() || !mapping->isInitialized())
    {
        return ActuatorStatus("", ActuatorStatus::State::ERROR);
    }

    std::string value;
    switch (mapping->getOutputType())
    {
    case RegisterMapping::OutputType::BOOL:
        value = readFromBoolMapping(mapping);
        break;
    case RegisterMapping::OutputType::UINT16:
        value = readFromUInt16Mapping(mapping);
        break;
    case RegisterMapping::OutputType::INT16:
        value = readFromInt16Mapping(mapping);
        break;
    case RegisterMapping::OutputType::UINT32:
        value = readFromUInt32Mapping(mapping);
        break;
    case RegisterMapping::OutputType::INT32:
        value = readFromInt32Mapping(mapping);
        break;
    case RegisterMapping::OutputType::FLOAT:
        value = readFromFloatMapping(mapping);
        break;
    case RegisterMapping::OutputType::STRING:
        value = readFromStringMapping(mapping);
        break;
    }
    return ActuatorStatus(value, ActuatorStatus::State::READY);
}

// getActuatorStatusFromCoil() - handles
wolkabout::ActuatorStatus ModbusBridge::getActuatorStatusFromCoil(const std::shared_ptr<RegisterMapping>& mapping)
{
    if (!mapping->isValid() || !mapping->isInitialized())
    {
        return ActuatorStatus("", ActuatorStatus::State::ERROR);
    }

    return ActuatorStatus(readFromBoolMapping(mapping), ActuatorStatus::State::READY);
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

    auto mapping = m_registerMappingByReference[deviceKey + '.' + reference];

    if (mapping->getRegisterType() != RegisterMapping::RegisterType::HOLDING_REGISTER &&
        mapping->getRegisterType() != RegisterMapping::RegisterType::COIL)
    {
        LOG(ERROR) << "ModbusBridge: Modbus register mapped to reference '" << reference
                   << "' can not be treated as actuator - Modbus register must be of type 'HOLDING_REGISTER' or 'COIL'";
        return ActuatorStatus("", ActuatorStatus::State::ERROR);
    }

    if (mapping->isReadRestricted())
        return ActuatorStatus("", ActuatorStatus::State::READY);

    return mapping->getRegisterType() == RegisterMapping::RegisterType::HOLDING_REGISTER ?
             getActuatorStatusFromHoldingRegister(mapping) :
             getActuatorStatusFromCoil(mapping);
}

// getConfiguration and helper methods
// getConfigurationStatusFromHoldingRegister() - handles
std::string ModbusBridge::getConfigurationStatusFromHoldingRegister(const std::shared_ptr<RegisterMapping>& mapping)
{
    switch (mapping->getOutputType())
    {
    case RegisterMapping::OutputType::BOOL:
        return (readFromBoolMapping(mapping));
    case RegisterMapping::OutputType::UINT16:
        return (readFromUInt16Mapping(mapping));
    case RegisterMapping::OutputType::INT16:
        return (readFromInt16Mapping(mapping));
    case RegisterMapping::OutputType::UINT32:
        return (readFromUInt32Mapping(mapping));
    case RegisterMapping::OutputType::INT32:
        return (readFromInt32Mapping(mapping));
    case RegisterMapping::OutputType::FLOAT:
        return (readFromFloatMapping(mapping));
    default:
        throw std::logic_error("ModbusBridge: Configuration mapping " + mapping->getReference() +
                               " cannot be of output type STRING.");
    }
}

// getConfigurationStatusFromCoil() - handles
std::string ModbusBridge::getConfigurationStatusFromCoil(const std::shared_ptr<RegisterMapping>& mapping)
{
    return readFromBoolMapping(mapping);
}

// getConfiguration() - decides on the handler to handle the status request
std::vector<ConfigurationItem> ModbusBridge::getConfiguration(const std::string& deviceKey)
{
    std::vector<ConfigurationItem> configurations;
    for (const auto& deviceConfigurations : m_configurationMappingByDeviceKey)
    {
        for (const auto& mappingRef : deviceConfigurations.second)
        {
            const auto& ref = mappingRef.substr(mappingRef.find(SEPARATOR) + 1);
            auto& mappings = m_configurationMappingByDeviceKeyAndRef[mappingRef];

            std::vector<std::string> values;
            for (const auto& mapping : mappings)
            {
                switch (mapping.second->getRegisterType())
                {
                case RegisterMapping::RegisterType::COIL:
                    values.emplace_back(getConfigurationStatusFromCoil(mapping.second));
                    break;
                case RegisterMapping::RegisterType::HOLDING_REGISTER:
                    values.emplace_back(getConfigurationStatusFromHoldingRegister(mapping.second));
                    break;
                default:
                    throw std::logic_error("ModbusReader: Configuration mapping " + ref +
                                           " is of invalid register type.");
                }
            }
            configurations.emplace_back(ConfigurationItem(values, ref));
        }
    }

    return configurations;
}

// getDeviceStatus() - handles status request about a specific device
wolkabout::DeviceStatus::Status ModbusBridge::getDeviceStatus(const std::string& deviceKey)
{
    return m_devicesStatusBySlaveAddress[getSlaveAddress(deviceKey)];
}
}    // namespace wolkabout
