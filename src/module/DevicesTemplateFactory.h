//
// Created by nvuletic on 2/19/20.
//

#ifndef WOLKGATEWAYMODBUSMODULE_DEVICESTEMPLATEFACTORY_H
#define WOLKGATEWAYMODBUSMODULE_DEVICESTEMPLATEFACTORY_H

#include "DevicesConfigurationTemplate.h"
#include "model/DeviceTemplate.h"

namespace wolkabout
{
class DevicesTemplateFactory
{
    static DataType getDataTypeFromRegisterType(ModbusRegisterMapping::RegisterType registerType);

    static bool processDefaultMapping(const ModbusRegisterMapping& mapping, const DataType& dataType,
                                      std::vector<SensorTemplate>& sensorTemplates,
                                      std::vector<ActuatorTemplate>& actuatorTemplates);

    static bool processSensorMapping(const ModbusRegisterMapping& mapping, const DataType& dataType,
                                     std::vector<SensorTemplate>& sensorTemplates);

    static bool processActuatorMapping(const ModbusRegisterMapping& mapping, const DataType& dataType,
                                       std::vector<ActuatorTemplate>& actuatorTemplates);

    static bool processAlarmMapping(const ModbusRegisterMapping& mapping, const DataType& dataType,
                                    std::vector<AlarmTemplate>& alarmTemplates);

    static bool processConfigurationMapping(const ModbusRegisterMapping& mapping, const DataType& dataType,
                                            std::vector<ConfigurationTemplate>& configurationTemplates);

    static std::unique_ptr<wolkabout::DeviceTemplate> makeTemplateFromDeviceConfigTemplate(
      DevicesConfigurationTemplate& configTemplate);
};
}    // namespace wolkabout

#endif    // WOLKGATEWAYMODBUSMODULE_DEVICESTEMPLATEFACTORY_H
