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
#include "utilities/Logger.h"

namespace wolkabout
{
// handleActuation and helper methods
// void handleActuatorForHoldingRegister() -
void ModbusBridge::handleActuationForHoldingRegister(RegisterMapping& mapping, const std::string& value)
{
    if (mapping.getDataType() == ModuleMapping::DataType::INT16)
    {
        short shortValue;
        try
        {
            shortValue = static_cast<short>(std::stoi(value));
        }
        catch (std::exception&)
        {
            LOG(ERROR)
              << "ModbusBridge: Unable to convert value from platform for holding register - Register address: "
              << mapping.getAddress() << " Value: " << value;
            mapping.setValid(false);
            return;
        }

        if (!m_modbusClient.writeHoldingRegister(static_cast<int>(mapping.getSlaveAddress()), mapping.getAddress(),
                                                 shortValue))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << mapping.getAddress() << " Value: " << value;
            mapping.setValid(false);
        }
    }
    else if (mapping.getDataType() == ModuleMapping::DataType::UINT16)
    {
        unsigned short unsignedShortValue;
        try
        {
            unsignedShortValue = static_cast<unsigned short>(std::stoi(value));
        }
        catch (std::exception&)
        {
            LOG(ERROR)
              << "ModbusBridge: Unable to convert value from platform for holding register - Register address: "
              << mapping.getAddress() << " Value: " << value;
            mapping.setValid(false);
            return;
        }

        if (!m_modbusClient.writeHoldingRegister(mapping.getSlaveAddress(), mapping.getAddress(), unsignedShortValue))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << mapping.getAddress() << " Value: " << value;
            mapping.setValid(false);
        }
    }
    else if (mapping.getDataType() == ModuleMapping::DataType::REAL32)
    {
        float floatValue;
        try
        {
            floatValue = std::stof(value);
        }
        catch (std::exception&)
        {
            LOG(ERROR)
              << "ModbusBridge: Unable to convert value from platform for holding register - Register address: "
              << mapping.getAddress() << " Value: " << value;
            mapping.setValid(false);
            return;
        }

        if (!m_modbusClient.writeHoldingRegister(mapping.getSlaveAddress(), mapping.getAddress(), floatValue))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << mapping.getAddress() << " Value: " << value;
            mapping.setValid(false);
        }
    }
}
// void handleActuatorForCoil() -
void ModbusBridge::handleActuationForCoil(RegisterMapping& mapping, const std::string& value)
{
    bool boolValue =
      std::any_of(TRUE_VALUES.begin(), TRUE_VALUES.end(), [&](const std::string& ref) { return ref == value; });

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
    if (mapping.getRegisterType() != ModuleMapping::RegisterType::HOLDING_REGISTER_ACTUATOR &&
        mapping.getRegisterType() != ModuleMapping::RegisterType::COIL)
    {
        LOG(ERROR)
          << "ModbusBridge: Modbus register mapped to reference '" << reference
          << "' can not be treated as actuator - Modbus register must be of type 'HOLDING_REGISTER_ACTUATOR' or 'COIL'";
        return;
    }

    // Pass through the appropriate handler
    if (mapping.getRegisterType() == ModuleMapping::RegisterType::HOLDING_REGISTER_ACTUATOR)
    {
        handleActuationForHoldingRegister(mapping, value);
    }
    else if (mapping.getRegisterType() == ModuleMapping::RegisterType::COIL)
    {
        handleActuationForCoil(mapping, value);
    }
}

// handleConfiguration and helper methods
// handleConfigurationForHoldingRegister() - handles
void ModbusBridge::handleConfigurationForHoldingRegister(RegisterMapping& mapping, const std::string& value)
{
    if (mapping.getDataType() == ModuleMapping::DataType::INT16)
    {
        short shortValue;
        try
        {
            shortValue = static_cast<short>(std::stoi(value));
        }
        catch (std::exception&)
        {
            LOG(ERROR)
              << "ModbusBridge: Unable to convert value from platform for holding register - Register address: "
              << mapping.getAddress() << " Value: " << value;
            mapping.setValid(false);
            return;
        }

        if (!m_modbusClient.writeHoldingRegister(mapping.getSlaveAddress(), mapping.getAddress(), shortValue))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << mapping.getAddress() << " Value: " << shortValue;
            mapping.setValid(false);
        }
    }
    else if (mapping.getDataType() == ModuleMapping::DataType::UINT16)
    {
        unsigned short unsignedShortValue;
        try
        {
            unsignedShortValue = static_cast<unsigned short>(std::stoi(value));
        }
        catch (std::exception&)
        {
            LOG(ERROR)
              << "ModbusBridge: Unable to convert value from platform for holding register - Register address: "
              << mapping.getAddress() << " Value: " << value;
            mapping.setValid(false);
            return;
        }

        if (!m_modbusClient.writeHoldingRegister(mapping.getSlaveAddress(), mapping.getAddress(), unsignedShortValue))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << mapping.getAddress() << " Value: " << unsignedShortValue;
            mapping.setValid(false);
        }
    }
    else if (mapping.getDataType() == ModuleMapping::DataType::REAL32)
    {
        float floatValue;
        try
        {
            floatValue = std::stof(value);
        }
        catch (std::exception&)
        {
            LOG(ERROR)
              << "ModbusBridge: Unable to convert value from platform for holding register - Register address: "
              << mapping.getAddress() << " Value: " << value;
            mapping.setValid(false);
            return;
        }

        if (!m_modbusClient.writeHoldingRegister(mapping.getSlaveAddress(), mapping.getAddress(), floatValue))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << mapping.getAddress() << " Value: " << floatValue;
            mapping.setValid(false);
        }
    }
}
// handleConfigurationForHoldingRegisters() - handles
void ModbusBridge::handleConfigurationForHoldingRegisters(RegisterMapping& mapping,
                                                          const std::vector<std::string>& value)
{
    if (mapping.getDataType() == ModuleMapping::DataType::INT16)
    {
        std::vector<short> shorts;
        shorts.reserve(value.size());
        for (const std::string& shortString : value)
        {
            try
            {
                shorts.push_back(static_cast<short>(std::stoi(shortString)));
            }
            catch (std::exception&)
            {
                LOG(ERROR)
                  << "ModbusBridge: Unable to convert value from platform for holding registers - Register address: "
                  << mapping.getAddress() << " Value: " << shortString;
                mapping.setValid(false);
                return;
            }
        }
        if (!m_modbusClient.writeHoldingRegisters(mapping.getSlaveAddress(), mapping.getAddress(), shorts))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << mapping.getAddress();
            mapping.setValid(false);
        }
    }
    else if (mapping.getDataType() == ModuleMapping::DataType::UINT16)
    {
        std::vector<unsigned short> unsignedShorts;
        unsignedShorts.reserve(value.size());
        for (const std::string& shortString : value)
        {
            try
            {
                unsignedShorts.push_back(static_cast<unsigned short>(std::stoi(shortString)));
            }
            catch (std::exception&)
            {
                LOG(ERROR)
                  << "ModbusBridge: Unable to convert value from platform for holding registers - Register address: "
                  << mapping.getAddress() << " Value: " << shortString;
                mapping.setValid(false);
                return;
            }
        }
        if (!m_modbusClient.writeHoldingRegisters(mapping.getSlaveAddress(), mapping.getAddress(), unsignedShorts))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << mapping.getAddress();
            mapping.setValid(false);
        }
    }
    else if (mapping.getDataType() == ModuleMapping::DataType::REAL32)
    {
        std::vector<float> floats;
        floats.reserve(value.size());
        for (const std::string& shortString : value)
        {
            try
            {
                floats.push_back(static_cast<float>(std::stof(shortString)));
            }
            catch (std::exception&)
            {
                LOG(ERROR)
                  << "ModbusBridge: Unable to convert value from platform for holding registers - Register address: "
                  << mapping.getAddress() << " Value: " << shortString;
                mapping.setValid(false);
                return;
            }
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
void ModbusBridge::handleConfigurationForCoil(RegisterMapping& mapping, const std::string& value)
{
    bool boolValue = std::find(TRUE_VALUES.begin(), TRUE_VALUES.end(), value) != TRUE_VALUES.end();
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

        if (mapping.getRegisterType() == ModuleMapping::RegisterType::COIL)
        {
            handleConfigurationForCoil(mapping, config.getValues()[0]);
        }
        else
        {
            if (!mapping.getLabelMap().empty())
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
}    // namespace wolkabout
