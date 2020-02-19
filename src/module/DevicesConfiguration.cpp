//
// Created by nvuletic on 2/19/20.
//

#include "DevicesConfiguration.h"

#include <iostream>

namespace wolkabout
{
DevicesConfiguration::DevicesConfiguration(nlohmann::json j) : m_templates(), m_devices()
{
    for (json::object_t templateJson : j["templates"].get<json::array_t>())
    {
        std::string templateName = templateJson["name"].get<std::string>();
        m_templates.insert(std::pair<std::string, std::unique_ptr<DevicesConfigurationTemplate>>(
          templateName, new DevicesConfigurationTemplate(templateJson)));
    }

    for (json::object_t deviceJson : j["devices"].get<json::array_t>())
    {
        std::string keyName = deviceJson["key"].get<std::string>();
        std::string templateName = deviceJson["template"].get<std::string>();

        if (m_templates.find(templateName) != m_templates.end())
        {
            auto pointer =
              reinterpret_cast<std::unique_ptr<DevicesConfigurationTemplate>*>(&(*m_templates.find(templateName)));
            m_devices.insert(std::pair<std::string, std::unique_ptr<DeviceInformation>>(
              keyName, new DeviceInformation(deviceJson, pointer)));
        }
        else
        {
            throw std::logic_error("Missing template " + templateName + " required by device " + keyName + ".");
        }
    }
}

const std::map<std::string, std::unique_ptr<DevicesConfigurationTemplate>>& DevicesConfiguration::getMTemplates() const
{
    return m_templates;
}

const std::map<std::string, std::unique_ptr<DeviceInformation>>& DevicesConfiguration::getMDevices() const
{
    return m_devices;
}
}    // namespace wolkabout
