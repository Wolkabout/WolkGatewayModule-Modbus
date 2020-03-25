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
// getActuatorStatus and helper methods
// getActuatorStatusFromHoldingRegister() - handles
wolkabout::ActuatorStatus ModbusBridge::getActuatorStatusFromHoldingRegister(RegisterMapping& mapping)
{
    if (mapping.getDataType() == ModuleMapping::DataType::INT16)
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
    else if (mapping.getDataType() == ModuleMapping::DataType::UINT16)
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
    else if (mapping.getDataType() == ModuleMapping::DataType::REAL32)
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
wolkabout::ActuatorStatus ModbusBridge::getActuatorStatusFromCoil(RegisterMapping& mapping)
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

    if (mapping.getRegisterType() != ModuleMapping::RegisterType::HOLDING_REGISTER_ACTUATOR &&
        mapping.getRegisterType() != ModuleMapping::RegisterType::COIL)
    {
        LOG(ERROR)
          << "ModbusBridge: Modbus register mapped to reference '" << reference
          << "' can not be treated as actuator - Modbus register must be of type 'HOLDING_REGISTER_ACTUATOR' or 'COIL'";
        return ActuatorStatus("", ActuatorStatus::State::ERROR);
    }

    return mapping.getRegisterType() == ModuleMapping::RegisterType::HOLDING_REGISTER_ACTUATOR ?
             getActuatorStatusFromHoldingRegister(mapping) :
             getActuatorStatusFromCoil(mapping);
}

// getConfiguration and helper methods
// getConfigurationStatusFromHoldingRegister() - handles
ConfigurationItem ModbusBridge::getConfigurationStatusFromHoldingRegister(RegisterMapping& mapping)
{
    std::vector<std::string> configurationValues;
    if (mapping.getDataType() == ModuleMapping::DataType::INT16)
    {
        if (!mapping.getLabelMap().empty())
        {
            std::vector<short> values;
            if (!m_modbusClient.readHoldingRegisters(mapping.getSlaveAddress(), mapping.getAddress(),
                                                     static_cast<int>(mapping.getLabelMap().size()), values))
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
    else if (mapping.getDataType() == ModuleMapping::DataType::UINT16)
    {
        if (!mapping.getLabelMap().empty())
        {
            std::vector<unsigned short> values;
            if (!m_modbusClient.readHoldingRegisters(mapping.getSlaveAddress(), mapping.getAddress(),
                                                     static_cast<int>(mapping.getLabelMap().size()), values))
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
    else if (mapping.getDataType() == ModuleMapping::DataType::REAL32)
    {
        if (!mapping.getLabelMap().empty())
        {
            std::vector<float> values;
            if (!m_modbusClient.readHoldingRegisters(mapping.getSlaveAddress(), mapping.getAddress(),
                                                     static_cast<int>(mapping.getLabelMap().size()), values))
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
ConfigurationItem ModbusBridge::getConfigurationStatusFromCoil(RegisterMapping& mapping)
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
        case ModuleMapping::RegisterType::COIL:
            configurations.emplace_back(getConfigurationStatusFromCoil(mapping));
            break;
        case ModuleMapping::RegisterType::HOLDING_REGISTER_ACTUATOR:
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
}    // namespace wolkabout
