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

#include "WolkaboutTemplateFactory.h"

namespace wolkabout
{
DataType WolkaboutTemplateFactory::getDataTypeFromRegisterType(RegisterMapping::RegisterType registerType)
{
    if (registerType == RegisterMapping::RegisterType::COIL ||
        registerType == RegisterMapping::RegisterType::INPUT_CONTACT)
    {
        return DataType::BOOLEAN;
    }
    else
    {
        return DataType::NUMERIC;
    }
}

bool WolkaboutTemplateFactory::processDefaultMapping(const RegisterMapping& mapping, const DataType& dataType,
                                                     std::vector<SensorTemplate>& sensorTemplates,
                                                     std::vector<ActuatorTemplate>& actuatorTemplates)
{
    switch (mapping.getRegisterType())
    {
    case RegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR:
    case RegisterMapping::RegisterType::COIL:
    {
        actuatorTemplates.emplace_back(mapping.getName(), mapping.getReference(), dataType, mapping.getDescription(),
                                       mapping.getMinimum(), mapping.getMaximum());
        return true;
    }
    case RegisterMapping::RegisterType::HOLDING_REGISTER_SENSOR:
    case RegisterMapping::RegisterType::INPUT_REGISTER:
    case RegisterMapping::RegisterType::INPUT_CONTACT:
    {
        sensorTemplates.emplace_back(mapping.getName(), mapping.getReference(), dataType, mapping.getDescription(),
                                     mapping.getMinimum(), mapping.getMaximum());
        return true;
    }
    default:
    {
        throw std::logic_error("WolkGatewayModbusModule Application: Mapping with reference '" +
                               mapping.getReference() + "' not added to device manifest - Unknown register type");
    }
    }
}

bool WolkaboutTemplateFactory::processSensorMapping(const RegisterMapping& mapping, const DataType& dataType,
                                                    std::vector<SensorTemplate>& sensorTemplates)
{
    switch (mapping.getRegisterType())
    {
    case RegisterMapping::RegisterType::INPUT_CONTACT:
    case RegisterMapping::RegisterType::INPUT_REGISTER:
    case RegisterMapping::RegisterType::HOLDING_REGISTER_SENSOR:
        sensorTemplates.emplace_back(mapping.getName(), mapping.getReference(), dataType, mapping.getDescription(),
                                     mapping.getMinimum(), mapping.getMaximum());
        return true;
    default:
        throw std::logic_error("WolkGatewayModbusModule Application: Mapping with reference '" +
                               mapping.getReference() +
                               "' not added to device manifest - Incompatible Mapping and Register type combination");
    }
}

bool WolkaboutTemplateFactory::processActuatorMapping(const RegisterMapping& mapping, const DataType& dataType,
                                                      std::vector<ActuatorTemplate>& actuatorTemplates)
{
    switch (mapping.getRegisterType())
    {
    case RegisterMapping::RegisterType::COIL:
    case RegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR:
        actuatorTemplates.emplace_back(mapping.getName(), mapping.getReference(), dataType, mapping.getDescription(),
                                       mapping.getMinimum(), mapping.getMaximum());
        return true;
    default:
        throw std::logic_error("WolkGatewayModbusModule Application: Mapping with reference '" +
                               mapping.getReference() +
                               "' not added to device manifest - Incompatible Mapping and Register type combination");
    }
}

bool WolkaboutTemplateFactory::processAlarmMapping(const RegisterMapping& mapping, const DataType& dataType,
                                                   std::vector<AlarmTemplate>& alarmTemplates)
{
    switch (mapping.getRegisterType())
    {
    case RegisterMapping::RegisterType::INPUT_CONTACT:
        alarmTemplates.emplace_back(mapping.getName(), mapping.getReference(), mapping.getDescription());
        return true;
    default:
        throw std::logic_error("WolkGatewayModbusModule Application: Mapping with reference '" +
                               mapping.getReference() +
                               "' not added to device manifest - Incompatible Mapping and Register type combination");
    }
}

bool WolkaboutTemplateFactory::processConfigurationMapping(const RegisterMapping& mapping, const DataType& dataType,
                                                           std::vector<ConfigurationTemplate>& configurationTemplates)
{
    switch (mapping.getRegisterType())
    {
    case RegisterMapping::RegisterType::COIL:
    case RegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR:
        if (mapping.getLabelsAndAddresses().empty())
        {
            configurationTemplates.emplace_back(mapping.getName(), mapping.getReference(), dataType,
                                                mapping.getDescription(), std::string(""), mapping.getMinimum(),
                                                mapping.getMaximum());
        }
        else
        {
            std::vector<std::string> labels;
            for (const auto& kvp : mapping.getLabelsAndAddresses())
            {
                labels.push_back(kvp.first);
            }
            configurationTemplates.emplace_back(mapping.getName(), mapping.getReference(), dataType,
                                                mapping.getDescription(), std::string(""), labels, mapping.getMinimum(),
                                                mapping.getMaximum());
        }
        return true;
    default:
        throw std::logic_error("WolkGatewayModbusModule Application: Mapping with reference '" +
                               mapping.getReference() +
                               "' not added to device manifest - Incompatible Mapping and Register type combination");
    }
}

std::unique_ptr<DeviceTemplate> WolkaboutTemplateFactory::makeTemplateFromDeviceConfigTemplate(
  DevicesConfigurationTemplate& configTemplate)
{
    std::vector<SensorTemplate> sensorTemplates;
    std::vector<ActuatorTemplate> actuatorTemplates;
    std::vector<AlarmTemplate> alarmTemplates;
    std::vector<ConfigurationTemplate> configurationTemplates;

    for (const RegisterMapping& mapping : configTemplate.getMappings())
    {
        auto mappingType = mapping.getMappingType();
        auto registerType = mapping.getRegisterType();

        DataType dataType = getDataTypeFromRegisterType(registerType);

        switch (mappingType)
        {
        case ModuleMapping::MappingType::DEFAULT:
            processDefaultMapping(mapping, dataType, sensorTemplates, actuatorTemplates);
            break;

        case ModuleMapping::MappingType::SENSOR:
            processSensorMapping(mapping, dataType, sensorTemplates);
            break;

        case ModuleMapping::MappingType::ACTUATOR:
            processActuatorMapping(mapping, dataType, actuatorTemplates);
            break;

        case ModuleMapping::MappingType::ALARM:
            processAlarmMapping(mapping, dataType, alarmTemplates);
            break;

        case ModuleMapping::MappingType::CONFIGURATION:
            processConfigurationMapping(mapping, dataType, configurationTemplates);
            break;

        default:
            break;
        }
    }

    return std::unique_ptr<DeviceTemplate>(new DeviceTemplate({configurationTemplates}, {sensorTemplates},
                                                              {alarmTemplates}, {actuatorTemplates}, "", {}, {}, {}));
}
}    // namespace wolkabout
