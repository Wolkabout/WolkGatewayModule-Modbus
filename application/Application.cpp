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

#include "Wolk.h"
#include "core/service/FirmwareInstaller.h"
#include "core/utilities/Logger.h"
#include "mappings/StringMapping.h"
#include "modbus/LibModbusSerialRtuClient.h"
#include "modbus/LibModbusTcpIpClient.h"
#include "model/DevicesConfiguration.h"
#include "model/ModuleConfiguration.h"
#include "module/ModbusBridge.h"
#include "module/WolkaboutTemplateFactory.h"
#include "utilities/JsonReaderParser.h"

#include <algorithm>
#include <chrono>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <utility>

std::map<std::string, std::unique_ptr<wolkabout::DeviceTemplate>> generateTemplates(
  wolkabout::DevicesConfiguration* devicesConfiguration)
{
    std::map<std::string, std::unique_ptr<wolkabout::DeviceTemplate>> templates;
    for (const auto& deviceTemplate : devicesConfiguration->getTemplates())
    {
        wolkabout::DevicesConfigurationTemplate& info = *deviceTemplate.second;
        templates.emplace(deviceTemplate.first,
                          wolkabout::WolkaboutTemplateFactory::makeTemplateFromDeviceConfigTemplate(info));
    }
    return templates;
}

void generateDevices(wolkabout::ModuleConfiguration* moduleConfiguration,
                     wolkabout::DevicesConfiguration* devicesConfiguration,
                     const std::map<std::string, std::unique_ptr<wolkabout::DeviceTemplate>>& templates,
                     std::map<int, std::unique_ptr<wolkabout::Device>>& devices,
                     std::map<std::string, std::vector<int>>& deviceTemplateMap)
{
    // Parse devices with templates to wolkabout::Device
    // We need to check, in the SERIAL/RTU, that all devices have a slave address
    // and that they're different from one another. In TCP/IP mode, we can have only
    // one device, so we need to check for that (and assign it a slaveAddress, because -1 is an invalid address).
    std::vector<int> occupiedSlaveAddresses;
    int assigningSlaveAddress = 1;

    for (const auto& deviceInformation : devicesConfiguration->getDevices())
    {
        wolkabout::DeviceInformation& info = *deviceInformation.second;
        if (moduleConfiguration->getConnectionType() == wolkabout::ModuleConfiguration::ConnectionType::SERIAL_RTU)
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
        const auto& pair = templates.find(templateName);
        if (pair != templates.end())
        {
            // Create the device with found template, push the slaveAddress as occupied
            wolkabout::DeviceTemplate& deviceTemplate = *pair->second;
            occupiedSlaveAddresses.push_back(info.getSlaveAddress());
            devices.emplace(info.getSlaveAddress(), std::unique_ptr<wolkabout::Device>(new wolkabout::Device(
                                                      info.getName(), info.getKey(), deviceTemplate)));

            // Emplace the template name in usedTemplates array for modbusBridge, and the slaveAddress
            if (deviceTemplateMap.find(templateName) != deviceTemplateMap.end())
            {
                deviceTemplateMap[templateName].emplace_back(info.getSlaveAddress());
            }
            else
            {
                deviceTemplateMap.emplace(templateName, std::vector<int>{info.getSlaveAddress()});
            }
        }
        else
        {
            LOG(WARN) << "Device " << info.getName() << " (" << info.getKey() << ") doesn't have a valid template!"
                      << "\nIgnoring device...";
        }
    }
}

int main(int argc, char** argv)
{
    // Setup logger
    wolkabout::Logger::init(wolkabout::LogLevel::TRACE, wolkabout::Logger::Type::CONSOLE);

    const auto& stringMapping = std::make_shared<wolkabout::StringMapping>(
      "STR1", wolkabout::RegisterMapping::RegisterType::HOLDING_REGISTER, std::vector<std::int32_t>{0, 1, 2},
      wolkabout::RegisterMapping::OperationType::STRINGIFY_ASCII);

    if (argc < 3)
    {
        LOG(ERROR) << "WolkGatewayModbusModule Application: Usage -  " << argv[0]
                   << " [moduleConfigurationFilePath] [devicesConfigurationFilePath]";
        return 1;
    }

    // Parse file passed in first arg - module configuration JSON file
    wolkabout::ModuleConfiguration moduleConfiguration(wolkabout::JsonReaderParser::readFile(argv[1]));

    // Parse file passed in second arg - devices configuration JSON file
    wolkabout::DevicesConfiguration devicesConfiguration(wolkabout::JsonReaderParser::readFile(argv[2]));

    // We need to do some checks to see if the inputted data is valid.
    // We don't want there to be no templates.
    if (devicesConfiguration.getTemplates().empty())
    {
        LOG(ERROR) << "You have not created any templates.";
        return -1;
    }

    // We don't want there to be no devices.
    if (devicesConfiguration.getDevices().empty())
    {
        LOG(ERROR) << "You have not created any devices.";
        return -1;
    }

    // Cut the execution right away if the user wants multiple TCP/IP devices.
    // TODO talk about future upgrade to support multiple TCP/IP connections/devices
    if (moduleConfiguration.getConnectionType() == wolkabout::ModuleConfiguration::ConnectionType::TCP_IP &&
        devicesConfiguration.getDevices().size() != 1)
    {
        LOG(ERROR) << "Application supports exactly one device in TCP/IP mode.";
        return -1;
    }

    // Warn the user if they're using more templates in TCP/IP.
    // Since TCP/IP supports only one device, you should have only one template.
    if (moduleConfiguration.getConnectionType() == wolkabout::ModuleConfiguration::ConnectionType::TCP_IP &&
        devicesConfiguration.getTemplates().size() != 1)
    {
        LOG(WARN) << "Using more than 1 template in TCP/IP mode is unnecessary. There can only be 1 TCP/IP device "
                  << "per module, which can use only one template.";
    }

    // Create the modbus client based on parsed information
    // Pass configuration parameters necessary to initialize the connection
    // according to the type of connection that the user required and setup.
    auto libModbusClient = [&]() -> std::unique_ptr<wolkabout::ModbusClient> {
        if (moduleConfiguration.getConnectionType() == wolkabout::ModuleConfiguration::ConnectionType::TCP_IP)
        {
            const auto& tcpConfiguration = moduleConfiguration.getTcpIpConfiguration();
            return std::unique_ptr<wolkabout::LibModbusTcpIpClient>(new wolkabout::LibModbusTcpIpClient(
              tcpConfiguration->getIp(), tcpConfiguration->getPort(), moduleConfiguration.getResponseTimeout()));
        }
        else if (moduleConfiguration.getConnectionType() == wolkabout::ModuleConfiguration::ConnectionType::SERIAL_RTU)
        {
            const auto& serialConfiguration = moduleConfiguration.getSerialRtuConfiguration();
            return std::unique_ptr<wolkabout::LibModbusSerialRtuClient>(new wolkabout::LibModbusSerialRtuClient(
              serialConfiguration->getSerialPort(), serialConfiguration->getBaudRate(),
              serialConfiguration->getDataBits(), serialConfiguration->getStopBits(),
              serialConfiguration->getBitParity(), moduleConfiguration.getResponseTimeout()));
        }

        throw std::logic_error("Unsupported Modbus implementation specified in module configuration file");
    }();

    std::map<std::string, std::unique_ptr<wolkabout::DeviceTemplate>> templates =
      generateTemplates(&devicesConfiguration);

    std::map<int, std::unique_ptr<wolkabout::Device>> devices;
    std::map<std::string, std::vector<int>> deviceTemplateMap;

    // Execute linking logic
    generateDevices(&moduleConfiguration, &devicesConfiguration, templates, devices, deviceTemplateMap);

    // If no devices are valid, the application has no reason to work
    if (devices.empty())
    {
        LOG(ERROR) << "No devices are valid. Quitting application...";
        return -1;
    }

    // Report the device count to the user
    LOG(INFO) << "Created " << devices.size() << " device(s)!";
    auto invalidDevices = devicesConfiguration.getDevices().size() - devices.size();
    if (invalidDevices > 0)
    {
        LOG(WARN) << "There were " << invalidDevices << " invalid device(s)!";
    }

    // Pass everything necessary to initialize the bridge
    LOG(DEBUG) << "Initializing the bridge...";
    auto modbusBridge = std::make_shared<wolkabout::ModbusBridge>(*libModbusClient, devicesConfiguration.getTemplates(),
                                                                  deviceTemplateMap, devices,
                                                                  moduleConfiguration.getRegisterReadPeriod());

    // Track if we registered or not
    bool registered = false;
    std::mutex mutex;
    std::condition_variable variable;
    std::unique_lock<std::mutex> lock{mutex};

    // Connect the bridge to Wolk instance
    LOG(DEBUG) << "Connecting with Wolk...";
    std::unique_ptr<wolkabout::Wolk> wolk =
      wolkabout::Wolk::newBuilder()
        .deviceStatusProvider(modbusBridge)
        .actuatorStatusProvider(modbusBridge)
        .actuationHandler(modbusBridge)
        .configurationProvider(modbusBridge)
        .configurationHandler(modbusBridge)
        .host(moduleConfiguration.getMqttHost())
        .withRegistrationResponseHandler([&](const std::string& deviceKey, wolkabout::PlatformResult::Code code) {
            registered = true;
            variable.notify_one();
        })
        .build();

    // Setup all the necessary callbacks for value changes from inside the modbusBridge
    modbusBridge->setOnSensorChange(
      [&](const std::string& deviceKey, const std::string& reference, const std::string& value) {
          wolk->addSensorReading(deviceKey, reference, value);
          wolk->publish();
      });

    modbusBridge->setOnActuatorStatusChange(
      [&](const std::string& deviceKey, const std::string& reference, const std::string& value) {
          wolk->publishActuatorStatus(deviceKey, reference, value);
      });

    modbusBridge->setOnAlarmChange([&](const std::string& deviceKey, const std::string& reference, bool active) {
        wolk->addAlarm(deviceKey, reference, active);
        wolk->publish();
    });

    modbusBridge->setOnConfigurationChange(
      [&](const std::string& deviceKey) { wolk->publishConfiguration(deviceKey); });

    modbusBridge->setOnDeviceStatusChange([&](const std::string& deviceKey, wolkabout::DeviceStatus::Status status) {
        wolk->publishDeviceStatus(deviceKey, status);
    });

    // Register all the devices created
    for (const auto& device : devices)
    {
        wolk->addDevice(*device.second);
    }

    wolk->connect(false);

    if (!registered)
    {
        variable.wait_for(lock, std::chrono::seconds(5), [&]() { return registered; });
    }

    modbusBridge->start();

    while (modbusBridge->isRunning())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
