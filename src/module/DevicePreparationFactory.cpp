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

#include "DevicePreparationFactory.h"
#include "DevicesTemplateFactory.h"
#include "ModuleConfiguration.h"
#include "utilities/Logger.h"

wolkabout::DevicePreparationFactory::DevicePreparationFactory(
  const std::map<std::string, std::unique_ptr<DevicesConfigurationTemplate>>& inputTemplates,
  const std::map<std::string, std::unique_ptr<DeviceInformation>>& inputDevices,
  ModuleConfiguration::ConnectionType connectionType)
: m_inputTemplates(inputTemplates), m_inputDevices(inputDevices), m_connectionType(connectionType)
{
    if (m_inputTemplates.empty())
    {
        throw std::logic_error("Input templates can\'t be empty!");
    }

    if (m_inputDevices.empty())
    {
        throw std::logic_error("Input devices can\'t be empty!");
    }

    // Execute linking logic

    // Parse templates to wolkabout::DeviceTemplate
    for (const auto& deviceTemplate : m_inputTemplates)
    {
        wolkabout::DevicesConfigurationTemplate& info = *deviceTemplate.second;
        m_templates.emplace(deviceTemplate.first,
                            wolkabout::DevicesTemplateFactory::makeTemplateFromDeviceConfigTemplate(info));
    }

    // Parse devices with templates to wolkabout::Device
    // We need to check, in the SERIAL/RTU, that all devices have a slave address
    // and that they're different from one another. In TCP/IP mode, we can have only
    // one device, so we need to check for that (and assign it a slaveAddress, because -1 is an invalid address).
    std::vector<int> occupiedSlaveAddresses;
    int assigningSlaveAddress = 1;

    for (const auto& deviceInformation : m_inputDevices)
    {
        wolkabout::DeviceInformation& info = *deviceInformation.second;
        if (connectionType == wolkabout::ModuleConfiguration::ConnectionType::SERIAL_RTU)
        {
            // If it doesn't at all have a slaveAddress or if the slaveAddress is already occupied
            // device is not valid.

            if (info.getSlaveAddress() == -1)
            {
                LOG(WARN) << "Device " << info.getName() << " (" << info.getKey() << ") is missing a slaveAddress!"
                          << "\nIgnoring device...";
                continue;
            }
            else if (std::any_of(occupiedSlaveAddresses.begin(), occupiedSlaveAddresses.end(),
                                 [&](int i) { return i == info.getSlaveAddress(); }))
            {
                LOG(WARN) << "Device " << info.getName() << " (" << info.getKey() << ") "
                          << "has a conflicting slaveAddress!\nIgnoring device...";
                continue;
            }
        }
        else
        {
            // If it's an TCP/IP device, assign it the next free slaveAddress.
            info.setSlaveAddress(assigningSlaveAddress++);
        }

        const std::string& templateName = info.getTemplateString();
        const auto& pair = m_templates.find(templateName);
        if (pair != m_templates.end())
        {
            // Create the device with found template, push the slaveAddress as occupied
            wolkabout::DeviceTemplate& deviceTemplate = *pair->second;
            occupiedSlaveAddresses.push_back(info.getSlaveAddress());
            m_devices.emplace(info.getSlaveAddress(),
                              new wolkabout::Device(info.getName(), info.getKey(), deviceTemplate));

            // Emplace the template name in usedTemplates array for modbusBridge, and the slaveAddress
            if (m_usedTemplates.find(templateName) != m_usedTemplates.end())
            {
                m_usedTemplates[templateName].emplace_back(info.getSlaveAddress());
            }
            else
            {
                m_usedTemplates.emplace(templateName, std::vector<int>{info.getSlaveAddress()});
            }
        }
        else
        {
            LOG(WARN) << "Device " << info.getName() << " (" << info.getKey() << ") doesn't have a valid template!"
                      << "\nIgnoring device...";
        }
    }
}

const std::map<std::string, std::unique_ptr<wolkabout::DeviceTemplate>>&
wolkabout::DevicePreparationFactory::getTemplates() const
{
    return m_templates;
}

const std::map<int, std::unique_ptr<wolkabout::Device>>& wolkabout::DevicePreparationFactory::getDevices() const
{
    return m_devices;
}

const std::map<std::string, std::vector<int>>& wolkabout::DevicePreparationFactory::getTemplateDeviceMap() const
{
    return m_usedTemplates;
}
