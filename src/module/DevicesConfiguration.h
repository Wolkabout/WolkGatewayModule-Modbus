//
// Created by nvuletic on 2/19/20.
//

#ifndef WOLKGATEWAYMODBUSMODULE_DEVICESCONFIGURATION_H
#define WOLKGATEWAYMODBUSMODULE_DEVICESCONFIGURATION_H

#include "DeviceInformation.h"

namespace wolkabout
{
using nlohmann::json;

class DevicesConfiguration
{
public:
    DevicesConfiguration() = default;

    explicit DevicesConfiguration(nlohmann::json j);

private:
    std::map<std::string, std::unique_ptr<DevicesConfigurationTemplate>> m_templates;
    std::map<std::string, std::unique_ptr<DeviceInformation>> m_devices;
};
}    // namespace wolkabout

#endif    // WOLKGATEWAYMODBUSMODULE_DEVICESCONFIGURATION_H
