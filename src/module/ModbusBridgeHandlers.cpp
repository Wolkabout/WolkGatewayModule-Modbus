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
#include "mappings/BoolMapping.h"
#include "mappings/FloatMapping.h"
#include "mappings/Int16Mapping.h"
#include "mappings/Int32Mapping.h"
#include "mappings/StringMapping.h"
#include "mappings/UInt16Mapping.h"
#include "mappings/UInt32Mapping.h"
#include "utilities/Logger.h"

namespace wolkabout
{
void ModbusBridge::writeToBoolMapping(RegisterMapping& mapping, const std::string& value)
{
    auto& boolMapping = dynamic_cast<BoolMapping&>(mapping);
    bool boolValue = std::find(TRUE_VALUES.begin(), TRUE_VALUES.end(), value) != TRUE_VALUES.end();
    boolMapping.writeValue(boolValue);
}

void ModbusBridge::writeToUInt16Mapping(RegisterMapping& mapping, const std::string& value)
{
    auto& uint16Mapping = dynamic_cast<UInt16Mapping&>(mapping);
    const auto uint16Value = static_cast<uint16_t>(std::stoul(value));
    uint16Mapping.writeValue(uint16Value);
}

void ModbusBridge::writeToInt16Mapping(RegisterMapping& mapping, const std::string& value)
{
    auto& int16Mapping = dynamic_cast<Int16Mapping&>(mapping);
    const auto int16Value = static_cast<int16_t>(std::stoi(value));
    int16Mapping.writeValue(int16Value);
}

void ModbusBridge::writeToUInt32Mapping(RegisterMapping& mapping, const std::string& value)
{
    auto& uint32Mapping = dynamic_cast<UInt32Mapping&>(mapping);
    const auto uint32Value = static_cast<uint32_t>(std::stoul(value));
    uint32Mapping.writeValue(uint32Value);
}

void ModbusBridge::writeToInt32Mapping(RegisterMapping& mapping, const std::string& value)
{
    auto& int32Mapping = dynamic_cast<Int32Mapping&>(mapping);
    const auto int32Value = static_cast<int32_t>(std::stoi(value));
    int32Mapping.writeValue(int32Value);
}

void ModbusBridge::writeToFloatMapping(RegisterMapping& mapping, const std::string& value)
{
    auto& floatMapping = dynamic_cast<FloatMapping&>(mapping);
    const auto floatValue = static_cast<float>(std::stof(value));
    floatMapping.writeValue(floatValue);
}

void ModbusBridge::writeToStringMapping(RegisterMapping& mapping, const std::string& value)
{
    auto& stringMapping = dynamic_cast<StringMapping&>(mapping);
    if (value.size() > (stringMapping.getRegisterCount() * 2))
    {
        LOG(WARN) << "ModbusBridge: Unsuccessful string write as string contains more than "
                  << stringMapping.getRegisterCount() * 2 << " characters on mapping " << mapping.getReference() << ".";
        return;
    }
    stringMapping.writeValue(value);
}

// handleActuation and helper methods
// void handleActuatorForHoldingRegister() -
void ModbusBridge::handleActuationForHoldingRegister(RegisterMapping& mapping, const std::string& value)
{
    switch (mapping.getOutputType())
    {
    case RegisterMapping::OutputType::BOOL:
        writeToBoolMapping(mapping, value);
        break;
    case RegisterMapping::OutputType::UINT16:
        writeToUInt16Mapping(mapping, value);
        break;
    case RegisterMapping::OutputType::INT16:
        writeToInt16Mapping(mapping, value);
        break;
    case RegisterMapping::OutputType::UINT32:
        writeToUInt32Mapping(mapping, value);
        break;
    case RegisterMapping::OutputType::INT32:
        writeToInt32Mapping(mapping, value);
        break;
    case RegisterMapping::OutputType::FLOAT:
        writeToFloatMapping(mapping, value);
        break;
    case RegisterMapping::OutputType::STRING:
        writeToStringMapping(mapping, value);
        break;
    }
}
// void handleActuatorForCoil() -
void ModbusBridge::handleActuationForCoil(RegisterMapping& mapping, const std::string& value)
{
    writeToBoolMapping(mapping, value);
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
    if (mapping.getRegisterType() != RegisterMapping::RegisterType::HOLDING_REGISTER &&
        mapping.getRegisterType() != RegisterMapping::RegisterType::COIL)
    {
        LOG(ERROR) << "ModbusBridge: Modbus register mapped to reference '" << reference
                   << "' can not be treated as actuator - Modbus register must be of type 'HOLDING_REGISTER' or 'COIL'";
        return;
    }

    // Pass through the appropriate handler
    if (mapping.getRegisterType() == RegisterMapping::RegisterType::HOLDING_REGISTER)
    {
        handleActuationForHoldingRegister(mapping, value);
    }
    else if (mapping.getRegisterType() == RegisterMapping::RegisterType::COIL)
    {
        handleActuationForCoil(mapping, value);
    }
}

// handleConfiguration and helper methods
// handleConfigurationForHoldingRegister() - handles
void ModbusBridge::handleConfigurationForHoldingRegister(RegisterMapping& mapping, const std::string& value)
{
    switch (mapping.getOutputType())
    {
    case RegisterMapping::OutputType::BOOL:
        writeToBoolMapping(mapping, value);
        break;
    case RegisterMapping::OutputType::UINT16:
        writeToUInt16Mapping(mapping, value);
        break;
    case RegisterMapping::OutputType::INT16:
        writeToInt16Mapping(mapping, value);
        break;
    case RegisterMapping::OutputType::UINT32:
        writeToUInt32Mapping(mapping, value);
        break;
    case RegisterMapping::OutputType::INT32:
        writeToInt32Mapping(mapping, value);
        break;
    case RegisterMapping::OutputType::FLOAT:
        writeToFloatMapping(mapping, value);
        break;
    default:
        LOG(ERROR) << "ModbusBridge: Illegal outputType set for CONFIGURATION mapping " << mapping.getReference()
                   << ".";
        break;
    }
}

// handleConfigurationForCoil() - handles
void ModbusBridge::handleConfigurationForCoil(RegisterMapping& mapping, const std::string& value)
{
    writeToBoolMapping(mapping, value);
}

// handleConfiguration() - decides on the handler to handle incoming data
void ModbusBridge::handleConfiguration(const std::string& deviceKey,
                                       const std::vector<ConfigurationItem>& configuration)
{
    int slaveAddress = getSlaveAddress(deviceKey);
    if (slaveAddress == -1 || m_deviceKeyBySlaveAddress.find(slaveAddress) == m_deviceKeyBySlaveAddress.end())
    {
        LOG(ERROR) << "ModbusBridge: No device with key '" << deviceKey << "'";
        return;
    }

    for (auto& config : configuration)
    {
        const std::string& reference = config.getReference();
        auto mappings = m_configurationMappingByDeviceKeyAndRef[deviceKey + '.' + reference];

        if (mappings.size() != config.getValues().size())
        {
            throw std::logic_error("ModbusBridge: Number of values received for Configuration " + reference +
                                   " doesn\'t match the mapping count.");
        }

        uint i = 0;
        for (const auto& mapping : mappings)
        {
            switch (mapping.second->getRegisterType())
            {
            case RegisterMapping::RegisterType::COIL:
                handleConfigurationForCoil(*mapping.second, config.getValues()[i++]);
                break;
            case RegisterMapping::RegisterType::HOLDING_REGISTER:
                handleConfigurationForHoldingRegister(*mapping.second, config.getValues()[i++]);
                break;
            default:
                throw std::logic_error("ModbusBridge: Illegal RegisterType for CONFIGURATION on Mapping : " +
                                       mapping.second->getReference());
            }
        }
    }
}
}    // namespace wolkabout
