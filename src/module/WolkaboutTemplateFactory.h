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

#ifndef WOLKGATEWAYMODBUSMODULE_WOLKABOUTTEMPLATEFACTORY_H
#define WOLKGATEWAYMODBUSMODULE_WOLKABOUTTEMPLATEFACTORY_H

#include "RegisterMapping.h"
#include "model/DeviceTemplate.h"
#include "model/DevicesConfigurationTemplate.h"

namespace wolkabout
{
/**
 * @brief Collection of utility methods used to create a DeviceTemplate (class used
 *        to register devices on the platform) from ModuleMappings, read from JSON.
 */
class WolkaboutTemplateFactory
{
private:
    /**
     * @brief Return the default data type for the mapping type.
     * @param mapping
     * @return data type that is default for the mapping
     */
    static DataType getDataTypeFromMapping(const ModuleMapping& mapping);

    /**
     * @brief Process json mapping where mapping type is default.
     * @param mapping
     * @param dataType outputting data type
     * @param sensorTemplates list of sensor templates
     * @param actuatorTemplates list of actuator templates
     * @return
     */
    static bool processDefaultMapping(const ModuleMapping& mapping, const DataType& dataType,
                                      std::vector<SensorTemplate>& sensorTemplates,
                                      std::vector<ActuatorTemplate>& actuatorTemplates);

    /**
     * @brief Process json mapping where mapping type is preset to SENSOR.
     * @param mapping
     * @param dataType outputting data type
     * @param sensorTemplates list of sensor templates
     * @return
     */
    static bool processSensorMapping(const ModuleMapping& mapping, const DataType& dataType,
                                     std::vector<SensorTemplate>& sensorTemplates);

    /**
     * @brief Process json mapping where mapping type is preset to ACTUATOR.
     * @param mapping
     * @param dataType outputting data type
     * @param actuatorTemplates list of actuator templates
     * @return
     */
    static bool processActuatorMapping(const ModuleMapping& mapping, const DataType& dataType,
                                       std::vector<ActuatorTemplate>& actuatorTemplates);

    /**
     * @brief Process json mapping where mapping type is preset to ALARM.
     * @param mapping
     * @param dataType outputting data type
     * @param alarmTemplates list of alarm templates
     * @return
     */
    static bool processAlarmMapping(const ModuleMapping& mapping, const DataType& dataType,
                                    std::vector<AlarmTemplate>& alarmTemplates);

    /**
     * @brief Process json mapping where mapping type is preset to CONFIGURATION.
     * @param mapping
     * @param dataType outputting data type
     * @param configurationTemplates list of configuration templates
     * @return
     */
    static bool processConfigurationMapping(const ModuleMapping& mapping, const DataType& dataType,
                                            std::vector<ConfigurationTemplate>& configurationTemplates);

public:
    /**
     * @brief Return the entire DeviceTemplate necessary for the Wolk instance to register the device.
     * @param configTemplate template passed from the configuration file.
     * @return DeviceTemplate ready for Wolk to register devices.
     */
    static std::unique_ptr<wolkabout::DeviceTemplate> makeTemplateFromDeviceConfigTemplate(
      DevicesConfigurationTemplate& configTemplate);
};
}    // namespace wolkabout

#endif    // WOLKGATEWAYMODBUSMODULE_WOLKABOUTTEMPLATEFACTORY_H
