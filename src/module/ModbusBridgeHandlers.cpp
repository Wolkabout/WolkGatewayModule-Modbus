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

#include <regex>

namespace wolkabout
{
void ModbusBridge::writeToMapping(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value)
{
    switch (mapping->getOutputType())
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

void ModbusBridge::writeToBoolMapping(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value)
{
    auto& boolMapping = (const std::shared_ptr<BoolMapping>&)mapping;
    try
    {
        bool boolValue = std::find(TRUE_VALUES.begin(), TRUE_VALUES.end(), value) != TRUE_VALUES.end();
        boolMapping->writeValue(boolValue);
    }
    catch (const std::exception& exception)
    {
        LOG(WARN) << "ModbusBridge: Issue occurred when trying to write value '" << value
                  << "' to a bool mapped register: " << boolMapping->getReference() << " -> '" << exception.what()
                  << "'.";
    }
}

void ModbusBridge::writeToUInt16Mapping(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value)
{
    auto& uint16Mapping = (const std::shared_ptr<UInt16Mapping>&)mapping;
    try
    {
        const auto uint16Value = static_cast<uint16_t>(std::stoul(value));
        uint16Mapping->writeValue(uint16Value);
    }
    catch (...)
    {
        LOG(WARN) << "ModbusBridge: Issue occurred when trying to write value '" << value
                  << "' to an uint16 mapped register: " << uint16Mapping->getReference();
    }
}

void ModbusBridge::writeToInt16Mapping(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value)
{
    auto& int16Mapping = (const std::shared_ptr<Int16Mapping>&)mapping;
    try
    {
        const auto int16Value = static_cast<int16_t>(std::stoi(value));
        int16Mapping->writeValue(int16Value);
    }
    catch (...)
    {
        LOG(WARN) << "ModbusBridge: Issue occurred when trying to write value '" << value
                  << "' to an int16 mapped register: " << int16Mapping->getReference();
    }
}

void ModbusBridge::writeToUInt32Mapping(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value)
{
    auto& uint32Mapping = (const std::shared_ptr<UInt32Mapping>&)mapping;
    try
    {
        const auto uint32Value = static_cast<uint32_t>(std::stoul(value));
        uint32Mapping->writeValue(uint32Value);
    }
    catch (...)
    {
        LOG(WARN) << "ModbusBridge: Issue occurred when trying to write value '" << value
                  << "' to an uint32 mapped register: " << uint32Mapping->getReference();
    }
}

void ModbusBridge::writeToInt32Mapping(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value)
{
    auto& int32Mapping = (const std::shared_ptr<Int32Mapping>&)mapping;
    try
    {
        const auto int32Value = static_cast<int32_t>(std::stoi(value));
        int32Mapping->writeValue(int32Value);
    }
    catch (...)
    {
        LOG(WARN) << "ModbusBridge: Issue occurred when trying to write value '" << value
                  << "' to an int32 mapped register: " << int32Mapping->getReference();
    }
}

void ModbusBridge::writeToFloatMapping(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value)
{
    auto& floatMapping = (const std::shared_ptr<FloatMapping>&)mapping;
    try
    {
        const auto floatValue = static_cast<float>(std::stof(value));
        floatMapping->writeValue(floatValue);
    }
    catch (...)
    {
        LOG(WARN) << "ModbusBridge: Issue occurred when trying to write value '" << value
                  << "' to a float mapped register: " << floatMapping->getReference();
    }
}

void ModbusBridge::writeToStringMapping(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value)
{
    auto& stringMapping = (const std::shared_ptr<StringMapping>&)mapping;
    if (value.size() > (stringMapping->getRegisterCount() * 2))
    {
        LOG(WARN) << "ModbusBridge: Unsuccessful string write as string contains more than "
                  << stringMapping->getRegisterCount() * 2 << " characters on mapping " << stringMapping->getReference()
                  << ".";
        return;
    }

    stringMapping->writeValue(value);
}

// handleActuation and helper methods
// void handleActuatorForHoldingRegister() -
void ModbusBridge::handleActuationForHoldingRegister(const std::shared_ptr<RegisterMapping>& mapping,
                                                     const std::string& value)
{
    writeToMapping(mapping, value);
}
// void handleActuatorForCoil() -
void ModbusBridge::handleActuationForCoil(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value)
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

    auto mapping = m_registerMappingByReference[deviceKey + '.' + reference];
    mapping->setSlaveAddress(slaveAddress);

    // Discard if the mapping is incorrect
    if (mapping->getRegisterType() != RegisterMapping::RegisterType::HOLDING_REGISTER &&
        mapping->getRegisterType() != RegisterMapping::RegisterType::COIL)
    {
        LOG(ERROR) << "ModbusBridge: Modbus register mapped to reference '" << reference
                   << "' can not be treated as actuator - Modbus register must be of type 'HOLDING_REGISTER' or 'COIL'";
        return;
    }

    // Pass through the appropriate handler
    if (mapping->getRegisterType() == RegisterMapping::RegisterType::HOLDING_REGISTER)
    {
        handleActuationForHoldingRegister(mapping, value);
    }
    else if (mapping->getRegisterType() == RegisterMapping::RegisterType::COIL)
    {
        handleActuationForCoil(mapping, value);
    }
}

// handleConfiguration and helper methods
// handleConfigurationForHoldingRegister() - handles
void ModbusBridge::handleConfigurationForHoldingRegister(const std::shared_ptr<RegisterMapping>& mapping,
                                                         const std::string& value)
{
    if (mapping->getOutputType() == RegisterMapping::OutputType::STRING)
    {
        LOG(ERROR) << "ModbusBridge: Illegal outputType set for CONFIGURATION mapping " << mapping->getReference()
                   << ".";
        return;
    }

    writeToMapping(mapping, value);
}

// handleConfigurationForCoil() - handles
void ModbusBridge::handleConfigurationForCoil(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value)
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
        const auto& reference = config.getReference();
        if (reference.find("DFV(") == 0 && reference.rfind(')') == reference.length() - 1)
        {
            // We have received a value for a default value
            LOG(INFO) << reference;
            const auto ref = reference.substr(reference.find("DFV(") + 4, reference.length() - 5);
            const auto value = config.getValues().front();

            // Find all devices that have the same device type and change it for them all!
            m_defaultValueMappingByReference[deviceKey + SEPARATOR + ref] = value;
            m_defaultValuePersistence->storeValue(deviceKey + SEPARATOR + ref, value);
            continue;
        }
        if (reference.find("RPW(") == 0 && reference.rfind(')') == reference.length() - 1)
        {
            // We have received a value for a repeated write
            LOG(INFO) << reference;
            const auto ref = reference.substr(reference.find("RPW(") + 4, reference.length() - 5);
            const auto value = config.getValues().front();

            // Check if the value can be parsed
            auto milliseconds = std::chrono::milliseconds{};
            try
            {
                milliseconds = std::chrono::milliseconds{std::stoull(value)};
            }
            catch (const std::exception& exception)
            {
                LOG(ERROR) << "Failed to accept a new `repeat` value for `" << deviceKey << "`/`" << ref
                           << "` - The value is not a valid number.";
                continue;
            }

            // Find all devices that have the same device type and change it for them all!
            m_repeatedWriteMappingByReference[deviceKey + SEPARATOR + ref] = milliseconds;
            m_registerMappingByReference[deviceKey + SEPARATOR + ref]->setRepeatedWrite(milliseconds);
            m_repeatValuePersistence->storeValue(deviceKey + SEPARATOR + ref, value);
            continue;
        }
        if (reference.find("SMV(") == 0 && reference.rfind(')') == reference.length() - 1)
        {
            // We have received a value for a safe mode value
            LOG(INFO) << reference;
            const auto ref = reference.substr(reference.find("SMV(") + 4, reference.length() - 5);
            const auto value = config.getValues().front();

            // Find all devices that have the same device type and change it for them all!
            m_safeModeMappingByReference[deviceKey + SEPARATOR + ref] = value;
            m_safeModePersistence->storeValue(deviceKey + SEPARATOR + ref, value);
            continue;
        }

        auto mappings = m_configurationMappingByDeviceKeyAndRef[deviceKey + SEPARATOR + reference];

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
                handleConfigurationForCoil(mapping.second, config.getValues()[i++]);
                break;
            case RegisterMapping::RegisterType::HOLDING_REGISTER:
                handleConfigurationForHoldingRegister(mapping.second, config.getValues()[i++]);
                break;
            default:
                throw std::logic_error("ModbusBridge: Illegal RegisterType for CONFIGURATION on Mapping : " +
                                       mapping.second->getReference());
            }
        }
    }
}
}    // namespace wolkabout
