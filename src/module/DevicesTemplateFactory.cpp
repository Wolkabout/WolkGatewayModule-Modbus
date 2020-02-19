//
// Created by nvuletic on 2/19/20.
//

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

std::unique_ptr<DeviceTemplate> DevicesTemplateFactory::makeTemplateFromDeviceConfigTemplate(
  DevicesConfigurationTemplate& configTemplate)
{
    std::vector<SensorTemplate> sensorTemplates;
    std::vector<ActuatorTemplate> actuatorTemplates;
    std::vector<AlarmTemplate> alarmTemplates;
    std::vector<ConfigurationTemplate> configurationTemplates;

    // TODO TIDY THIS UP!
    for (const ModbusRegisterMapping& mapping : configTemplate.getMappings())
    {
        auto mappingType = mapping.getMappingType();
        auto registerType = mapping.getRegisterType();

        DataType dataType = getDataTypeFromRegisterType(registerType);

        switch (mappingType)
        {
        case ModbusRegisterMapping::MappingType::DEFAULT:
            switch (registerType)
            {
            case ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR:
            {
                actuatorTemplates.emplace_back(mapping.getName(), mapping.getReference(), dataType,
                                               mapping.getDescription(), mapping.getMinimum(), mapping.getMaximum());
                break;
            }

            case ModbusRegisterMapping::RegisterType::COIL:
            {
                actuatorTemplates.emplace_back(mapping.getName(), mapping.getReference(), dataType,
                                               mapping.getDescription(), mapping.getMinimum(), mapping.getMaximum());
                break;
            }

            case ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_SENSOR:
            {
                sensorTemplates.emplace_back(mapping.getName(), mapping.getReference(), dataType,
                                             mapping.getDescription(), mapping.getMinimum(), mapping.getMaximum());
                break;
            }

            case ModbusRegisterMapping::RegisterType::INPUT_REGISTER:
            {
                sensorTemplates.emplace_back(mapping.getName(), mapping.getReference(), dataType,
                                             mapping.getDescription(), mapping.getMinimum(), mapping.getMaximum());
                break;
            }

            case ModbusRegisterMapping::RegisterType::INPUT_CONTACT:
            {
                sensorTemplates.emplace_back(mapping.getName(), mapping.getReference(), dataType,
                                             mapping.getDescription(), mapping.getMinimum(), mapping.getMaximum());
                break;
            }

            default:
            {
                throw std::logic_error("WolkGatewayModbusModule Application: Mapping with reference '" +
                                       mapping.getReference() +
                                       "' not added to device manifest - Unknown register type");
            }
            }
            break;

        case ModbusRegisterMapping::MappingType::SENSOR:
            switch (registerType)
            {
            case ModbusRegisterMapping::RegisterType::INPUT_CONTACT:
            case ModbusRegisterMapping::RegisterType::INPUT_REGISTER:
            case ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_SENSOR:
                sensorTemplates.emplace_back(mapping.getName(), mapping.getReference(), dataType,
                                             mapping.getDescription(), mapping.getMinimum(), mapping.getMaximum());
                break;
            default:
                throw std::logic_error(
                  "WolkGatewayModbusModule Application: Mapping with reference '" + mapping.getReference() +
                  "' not added to device manifest - Incompatible Mapping and Register type combination");
            }
            break;

        case ModbusRegisterMapping::MappingType::ACTUATOR:
            switch (registerType)
            {
            case ModbusRegisterMapping::RegisterType::COIL:
            case ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR:
                actuatorTemplates.emplace_back(mapping.getName(), mapping.getReference(), dataType,
                                               mapping.getDescription(), mapping.getMinimum(), mapping.getMaximum());
                break;
            default:
                throw std::logic_error(
                  "WolkGatewayModbusModule Application: Mapping with reference '" + mapping.getReference() +
                  "' not added to device manifest - Incompatible Mapping and Register type combination");
            }
            break;

        case ModbusRegisterMapping::MappingType::ALARM:
            switch (registerType)
            {
            case ModbusRegisterMapping::RegisterType::INPUT_CONTACT:
                alarmTemplates.emplace_back(mapping.getName(), mapping.getReference(), mapping.getDescription());
                break;
            default:
                throw std::logic_error(
                  "WolkGatewayModbusModule Application: Mapping with reference '" + mapping.getReference() +
                  "' not added to device manifest - Incompatible Mapping and Register type combination");
            }
            break;

        case ModbusRegisterMapping::MappingType::CONFIGURATION:
            switch (registerType)
            {
            case ModbusRegisterMapping::RegisterType::COIL:
            case ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR:
                if (mapping.getLabelsAndAddresses().empty())
                {
                    configurationTemplates.emplace_back(mapping.getName(), mapping.getReference(), dataType,
                                                        mapping.getDescription(), std::string(""), mapping.getMinimum(),
                                                        mapping.getMaximum());
                }
                else
                {
                    std::vector<std::string> labels;
                    for (auto const& kvp : mapping.getLabelsAndAddresses())
                    {
                        labels.push_back(kvp.first);
                    }
                    configurationTemplates.emplace_back(mapping.getName(), mapping.getReference(), dataType,
                                                        mapping.getDescription(), std::string(""), labels,
                                                        mapping.getMinimum(), mapping.getMaximum());
                }
                break;
            default:
                throw std::logic_error(
                  "WolkGatewayModbusModule Application: Mapping with reference '" + mapping.getReference() +
                  "' not added to device manifest - Incompatible Mapping and Register type combination");
            }
            break;

        default:
            break;
        }
    }

    return std::unique_ptr<DeviceTemplate>(new DeviceTemplate({configurationTemplates}, {sensorTemplates},
                                                              {alarmTemplates}, {actuatorTemplates}, "", {}, {}, {}));
}
}    // namespace wolkabout
