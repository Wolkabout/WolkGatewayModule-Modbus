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

#include "DevicesTemplateFactory.h"

namespace wolkabout
{
DataType DevicesTemplateFactory::getDataTypeFromRegisterType(ModbusRegisterMapping::RegisterType registerType)
{
    if (registerType == ModbusRegisterMapping::RegisterType::COIL ||
        registerType == ModbusRegisterMapping::RegisterType::INPUT_CONTACT)
    {
        return DataType::BOOLEAN;
    }
    else
    {
        return DataType::NUMERIC;
    }
}

bool DevicesTemplateFactory::processDefaultMapping(const ModbusRegisterMapping& mapping, const DataType& dataType,
                                                   const std::shared_ptr<DeviceTemplate>& deviceTemplate)
{
    switch (mapping.getRegisterType())
    {
    case ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR:
    case ModbusRegisterMapping::RegisterType::COIL:
    {
        deviceTemplate->addActuator(ActuatorTemplate(mapping.getName(), mapping.getReference(), dataType,
                                                     mapping.getDescription(), mapping.getMinimum(),
                                                     mapping.getMaximum()));
        return true;
    }
    case ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_SENSOR:
    case ModbusRegisterMapping::RegisterType::INPUT_REGISTER:
    case ModbusRegisterMapping::RegisterType::INPUT_CONTACT:
    {
        deviceTemplate->addSensor(SensorTemplate(mapping.getName(), mapping.getReference(), dataType,
                                                 mapping.getDescription(), mapping.getMinimum(), mapping.getMaximum()));
        return true;
    }
    default:
    {
        throw std::logic_error("WolkGatewayModbusModule Application: Mapping with reference '" +
                               mapping.getReference() + "' not added to device manifest - Unknown register type");
    }
    }
}

bool DevicesTemplateFactory::processSensorMapping(const ModbusRegisterMapping& mapping, const DataType& dataType,
                                                  const std::shared_ptr<DeviceTemplate>& deviceTemplate)
{
    switch (mapping.getRegisterType())
    {
    case ModbusRegisterMapping::RegisterType::INPUT_CONTACT:
    case ModbusRegisterMapping::RegisterType::INPUT_REGISTER:
    case ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_SENSOR:
        deviceTemplate->addSensor(SensorTemplate(mapping.getName(), mapping.getReference(), dataType,
                                                 mapping.getDescription(), mapping.getMinimum(), mapping.getMaximum()));
        return true;
    default:
        throw std::logic_error("WolkGatewayModbusModule Application: Mapping with reference '" +
                               mapping.getReference() +
                               "' not added to device manifest - Incompatible Mapping and Register type combination");
    }
}

bool DevicesTemplateFactory::processActuatorMapping(const ModbusRegisterMapping& mapping, const DataType& dataType,
                                                    const std::shared_ptr<DeviceTemplate>& deviceTemplate)
{
    switch (mapping.getRegisterType())
    {
    case ModbusRegisterMapping::RegisterType::COIL:
    case ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR:
        deviceTemplate->addActuator(ActuatorTemplate(mapping.getName(), mapping.getReference(), dataType,
                                                     mapping.getDescription(), mapping.getMinimum(),
                                                     mapping.getMaximum()));
        return true;
    default:
        throw std::logic_error("WolkGatewayModbusModule Application: Mapping with reference '" +
                               mapping.getReference() +
                               "' not added to device manifest - Incompatible Mapping and Register type combination");
    }
}

bool DevicesTemplateFactory::processAlarmMapping(const ModbusRegisterMapping& mapping, const DataType& dataType,
                                                 std::shared_ptr<DeviceTemplate> deviceTemplate)
{
    switch (mapping.getRegisterType())
    {
    case ModbusRegisterMapping::RegisterType::INPUT_CONTACT:
        deviceTemplate->addAlarm(AlarmTemplate(mapping.getName(), mapping.getReference(), mapping.getDescription()));
        return true;
    default:
        throw std::logic_error("WolkGatewayModbusModule Application: Mapping with reference '" +
                               mapping.getReference() +
                               "' not added to device manifest - Incompatible Mapping and Register type combination");
    }
}

bool DevicesTemplateFactory::processConfigurationMapping(const ModbusRegisterMapping& mapping, const DataType& dataType,
                                                         const std::shared_ptr<DeviceTemplate>& deviceTemplate)
{
    switch (mapping.getRegisterType())
    {
    case ModbusRegisterMapping::RegisterType::COIL:
    case ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR:
        if (mapping.getLabelsAndAddresses().empty())
        {
            deviceTemplate->addConfiguration(ConfigurationTemplate(mapping.getName(), mapping.getReference(), dataType,
                                                                   mapping.getDescription(), std::string(""),
                                                                   mapping.getMinimum(), mapping.getMaximum()));
        }
        else
        {
            std::vector<std::string> labels;
            for (const auto& kvp : mapping.getLabelsAndAddresses())
            {
                labels.push_back(kvp.first);
            }

            deviceTemplate->addConfiguration(ConfigurationTemplate(mapping.getName(), mapping.getReference(), dataType,
                                                                   mapping.getDescription(), std::string(""), labels,
                                                                   mapping.getMinimum(), mapping.getMaximum()));
        }
        return true;
    default:
        throw std::logic_error("WolkGatewayModbusModule Application: Mapping with reference '" +
                               mapping.getReference() +
                               "' not added to device manifest - Incompatible Mapping and Register type combination");
    }
}

std::unique_ptr<DeviceTemplate> DevicesTemplateFactory::makeTemplateFromDeviceConfigTemplate(
  DevicesConfigurationTemplate& configTemplate)
{
    std::vector<SensorTemplate> sensorTemplates;
    std::vector<ActuatorTemplate> actuatorTemplates;
    std::vector<AlarmTemplate> alarmTemplates;
    std::vector<ConfigurationTemplate> configurationTemplates;

    const auto pointer = std::make_shared<DeviceTemplate>();

    for (const ModbusRegisterMapping& mapping : configTemplate.getMappings())
    {
        auto mappingType = mapping.getMappingType();
        auto registerType = mapping.getRegisterType();

        DataType dataType = getDataTypeFromRegisterType(registerType);

        switch (mappingType)
        {
        case ModbusRegisterMapping::MappingType::DEFAULT:
            processDefaultMapping(mapping, dataType, pointer);
            break;

        case ModbusRegisterMapping::MappingType::SENSOR:
            processSensorMapping(mapping, dataType, pointer);
            break;

        case ModbusRegisterMapping::MappingType::ACTUATOR:
            processActuatorMapping(mapping, dataType, pointer);
            break;

        case ModbusRegisterMapping::MappingType::ALARM:
            processAlarmMapping(mapping, dataType, pointer);
            break;

        case ModbusRegisterMapping::MappingType::CONFIGURATION:
            processConfigurationMapping(mapping, dataType, pointer);
            break;

        default:
            break;
        }
    }

    return std::unique_ptr<DeviceTemplate>(new DeviceTemplate({configurationTemplates}, {sensorTemplates},
                                                              {alarmTemplates}, {actuatorTemplates}, "", {}, {}, {}));
}
}    // namespace wolkabout
