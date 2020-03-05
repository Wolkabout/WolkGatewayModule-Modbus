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
#include "modbus/LibModbusSerialRtuClient.h"
#include "modbus/LibModbusTcpIpClient.h"
#include "modbus/ModbusBridge.h"
#include "modbus/ModbusClient.h"
#include "module/DevicePreparationFactory.h"
#include "module/DevicesConfiguration.h"
#include "module/ModuleConfiguration.h"
#include "service/FirmwareInstaller.h"
#include "utilities/ConsoleLogger.h"
#include "utility/JsonReaderParser.h"

#include <algorithm>
#include <chrono>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <utility>

int main(int argc, char** argv)
{
    // Setup logger
    auto logger = std::unique_ptr<wolkabout::ConsoleLogger>(new wolkabout::ConsoleLogger());
    logger->setLogLevel(wolkabout::LogLevel::INFO);
    wolkabout::Logger::setInstance(std::move(logger));

    if (argc < 3)
    {
        LOG(ERROR) << "WolkGatewayModbusModule Application: Usage -  " << argv[0]
                   << " [moduleConfigurationFilePath] [devicesConfigurationFilePath]";
        return -1;
    }

    // Parse file passed in first arg - module configuration JSON file
    wolkabout::ModuleConfiguration moduleConfiguration(wolkabout::JsonReaderParser::readFile(argv[1]));

    // Parse file passed in second arg - devices configuration JSON file
    wolkabout::DevicesConfiguration devicesConfiguration(wolkabout::JsonReaderParser::readFile(argv[2]));

    // We need to do some checks to see if the inputted data is valid.
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
                  << "per module, which can use only one template "
                  << "We recommend using 1 template to improve performance.";
    }

    // Create the modbus client based on parsed information
    // Pass configuration parameters necessary to initialize the connection
    // according to the type of connection that the user required and setup.
    auto libModbusClient = [&]() -> std::unique_ptr<wolkabout::ModbusClient> {
        if (moduleConfiguration.getConnectionType() == wolkabout::ModuleConfiguration::ConnectionType::TCP_IP)
        {
            auto const& tcpConfiguration = moduleConfiguration.getTcpIpConfiguration();
            return std::unique_ptr<wolkabout::LibModbusTcpIpClient>(new wolkabout::LibModbusTcpIpClient(
              tcpConfiguration->getIp(), tcpConfiguration->getPort(), moduleConfiguration.getResponseTimeout()));
        }
        else if (moduleConfiguration.getConnectionType() == wolkabout::ModuleConfiguration::ConnectionType::SERIAL_RTU)
        {
            auto const& serialConfiguration = moduleConfiguration.getSerialRtuConfiguration();
            return std::unique_ptr<wolkabout::LibModbusSerialRtuClient>(new wolkabout::LibModbusSerialRtuClient(
              serialConfiguration->getSerialPort(), serialConfiguration->getBaudRate(),
              serialConfiguration->getDataBits(), serialConfiguration->getStopBits(),
              serialConfiguration->getBitParity(), moduleConfiguration.getResponseTimeout()));
        }

        throw std::logic_error("Unsupported Modbus implementation specified in module configuration file");
    }();

    // Process the devices configuration and create and register devices.
    wolkabout::DevicePreparationFactory factory(devicesConfiguration.getTemplates(), devicesConfiguration.getDevices(),
                                                moduleConfiguration.getConnectionType());
    const auto& templates = factory.getTemplates();
    const auto& devices = factory.getDevices();
    const auto& deviceTemplateMap = factory.getTemplateDeviceMap();

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

    // Connect the bridge to Wolk instance
    LOG(DEBUG) << "Connecting with Wolk...";
    std::unique_ptr<wolkabout::Wolk> wolk = wolkabout::Wolk::newBuilder()
                                              .deviceStatusProvider(modbusBridge)
                                              .actuatorStatusProvider(modbusBridge)
                                              .actuationHandler(modbusBridge)
                                              .configurationProvider(modbusBridge)
                                              .configurationHandler(modbusBridge)
                                              .host(moduleConfiguration.getMqttHost())
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
      [&](const std::string& deviceKey, std::vector<wolkabout::ConfigurationItem>& values) {
          wolk->publishConfiguration(deviceKey, values);
      });

    modbusBridge->setOnDeviceStatusChange([&](const std::string& deviceKey, wolkabout::DeviceStatus::Status status) {
        wolk->publishDeviceStatus(deviceKey, status);
    });

    // Register all the devices created
    for (auto const& device : devices)
    {
        wolk->addDevice(*device.second);
    }

    wolk->connect();
    modbusBridge->start();

    while (modbusBridge->isRunning())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
