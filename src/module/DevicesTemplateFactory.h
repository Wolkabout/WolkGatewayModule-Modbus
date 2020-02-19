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

    static std::unique_ptr<wolkabout::DeviceTemplate> makeTemplateFromDeviceConfigTemplate(
      DevicesConfigurationTemplate& configTemplate);
};
}    // namespace wolkabout

#endif    // WOLKGATEWAYMODBUSMODULE_DEVICESTEMPLATEFACTORY_H
