/*
 * Copyright 2018 WolkAbout Technology s.r.o.
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

#include "JsonReaderParser.h"
#include "Wolk.h"
#include "modbus/LibModbusSerialRtuClient.h"
#include "modbus/LibModbusTcpIpClient.h"
#include "modbus/ModbusBridge.h"
#include "modbus/ModbusClient.h"
#include "modbus/ModbusRegisterGroup.h"
#include "modbus/ModbusRegisterMapping.h"
#include "model/ActuatorTemplate.h"
#include "model/DeviceTemplate.h"
#include "model/SensorTemplate.h"
#include "module/DevicesConfiguration.h"
#include "module/ModuleConfiguration.h"
#include "service/FirmwareInstaller.h"
#include "utilities/ConsoleLogger.h"

#include <algorithm>
#include <chrono>
#include <map>
#include <memory>
#include <module/DevicesTemplateFactory.h>
#include <random>
#include <string>
#include <thread>
#include <utility>

bool loadModuleConfiguration(const std::string& file, wolkabout::ModuleConfiguration& moduleConfiguration)
{
    try
    {
        moduleConfiguration = wolkabout::ModuleConfiguration(wolkabout::JsonReaderParser::readFile(file));
        LOG(INFO) << "WolkGatewayModbusModule Application: Loaded module configuration from '" << file << "'";
        return true;
    }
    catch (std::logic_error& e)
    {
        LOG(ERROR) << "WolkGatewayModbusModule Application: Unable to parse module configuration file."
                   << "Reason: " << e.what();
        return false;
    }
}

bool loadDevicesConfiguration(const std::string& file, wolkabout::DevicesConfiguration& devicesConfiguration)
{
    try
    {
        devicesConfiguration = wolkabout::DevicesConfiguration(wolkabout::JsonReaderParser::readFile(file));
        LOG(INFO) << "WolkGatewayModbusModule Application: Loaded devices configuration from '" << file << "'";
        return true;
    }
    catch (std::logic_error& e)
    {
        LOG(ERROR) << "WolkGatewayModbusModule Application: Unable to parse devices configuration file."
                   << "Reason: " << e.what();
        return false;
    }
}

int main(int argc, char** argv)
{
    // Setup logger
    auto logger = std::unique_ptr<wolkabout::ConsoleLogger>(new wolkabout::ConsoleLogger());
    logger->setLogLevel(wolkabout::LogLevel::TRACE);
    wolkabout::Logger::setInstance(std::move(logger));

    // Checking if arg count is valid
    if (argc < 3)
    {
        LOG(ERROR) << "WolkGatewayModbusModule Application: Usage -  " << argv[0]
                   << " [moduleConfigurationFilePath] [devicesConfigurationFilePath]";
        return -1;
    }

    // Parse file passed in first arg - module configuration JSON file
    wolkabout::ModuleConfiguration moduleConfiguration;
    if (!loadModuleConfiguration(argv[1], moduleConfiguration))
    {
        return -1;
    }

    // Parse file passed in second arg - devices configuration JSON file
    wolkabout::DevicesConfiguration devicesConfiguration;
    if (!loadDevicesConfiguration(argv[2], devicesConfiguration))
    {
        return -1;
    }

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
            auto tcpConfiguration = moduleConfiguration.getTcpIpConfiguration();
            return std::unique_ptr<wolkabout::LibModbusTcpIpClient>(new wolkabout::LibModbusTcpIpClient(
              tcpConfiguration->getIp(), tcpConfiguration->getPort(), moduleConfiguration.getResponseTimeout()));
        }
        else if (moduleConfiguration.getConnectionType() == wolkabout::ModuleConfiguration::ConnectionType::SERIAL_RTU)
        {
            auto serialConfiguration = moduleConfiguration.getSerialRtuConfiguration();
            return std::unique_ptr<wolkabout::LibModbusSerialRtuClient>(new wolkabout::LibModbusSerialRtuClient(
              serialConfiguration->getSerialPort(), serialConfiguration->getBaudRate(),
              serialConfiguration->getDataBits(), serialConfiguration->getStopBits(),
              serialConfiguration->getBitParity(), moduleConfiguration.getResponseTimeout()));
        }

        throw std::logic_error("Unsupported Modbus implementation specified in module configuration file");
    }();

    // Process the devices configuration and create and register devices.
    std::map<std::string, std::unique_ptr<wolkabout::DeviceTemplate>> templates;
    std::map<std::string, std::unique_ptr<wolkabout::Device>> devices;

    // Parse templates to wolkabout::DeviceTemplate
    for (auto const& deviceTemplate : devicesConfiguration.getTemplates())
    {
        wolkabout::DevicesConfigurationTemplate& info = *deviceTemplate.second;
        templates.insert(std::pair<std::string, std::unique_ptr<wolkabout::DeviceTemplate>>(
          deviceTemplate.first, wolkabout::DevicesTemplateFactory::makeTemplateFromDeviceConfigTemplate(info)));
    }

    // Parse devices with templates to wolkabout::Device
    // We need to check, in the SERIAL/RTU, that all devices have a slave address
    // and that they're different from one another. In TCP/IP mode, we can have only
    // one device, so we need to check for that.
    std::vector<int> occupiedSlaveAddresses;

    for (auto const& deviceInformation : devicesConfiguration.getDevices())
    {
        wolkabout::DeviceInformation& info = *deviceInformation.second;
        if (moduleConfiguration.getConnectionType() == wolkabout::ModuleConfiguration::ConnectionType::SERIAL_RTU)
        {
            // If it doesn't at all have a slaveAddress or if the slaveAddress is already occupied
            // device is not valid.
            if (info.getSlaveAddress() == -1)
            {
                LOG(WARN) << "Device " << info.getName() << " (" << info.getKey() << ") is missing a slaveAddress!"
                          << "\nIgnoring device...";
                continue;
            }
            else if (std::find(occupiedSlaveAddresses.begin(), occupiedSlaveAddresses.end(), info.getSlaveAddress()) !=
                     occupiedSlaveAddresses.end())
            {
                LOG(WARN) << "Device " << info.getName() << " (" << info.getKey() << ") "
                          << "has a conflicting slaveAddress!\nIgnoring device...";
                continue;
            }
        }

        const std::string& templateName = info.getTemplateString();
        if (templates.find(templateName) != templates.end())
        {
            // Create the device with found template
            // Push the slaveAddress as occupied
            wolkabout::DeviceTemplate& deviceTemplate = *(templates.find(templateName)->second);
            occupiedSlaveAddresses.push_back(info.getSlaveAddress());
            devices.insert(std::pair<std::string, std::unique_ptr<wolkabout::Device>>(
              info.getKey(), new wolkabout::Device(info.getName(), info.getKey(), deviceTemplate)));
        }
        else
        {
            LOG(WARN) << "Device " << info.getName() << " (" << info.getKey() << ") doesn't have a valid template!"
                      << "\nIgnoring device...";
        }
    }

    LOG(DEBUG) << "Created " << devices.size() << " device(s)!";

    //    auto modbusBridge = std::make_shared<wolkabout::ModbusBridge>(*libModbusClient,
    //    std::move(modbusRegisterMappings), modbusConfiguration.getReadPeriod());
    //
    //    std::unique_ptr<wolkabout::Wolk> wolk = wolkabout::Wolk::newBuilder()
    //                                              .deviceStatusProvider(modbusBridge)
    //                                              .actuatorStatusProvider(modbusBridge)
    //                                              .actuationHandler(modbusBridge)
    //                                              .configurationProvider(modbusBridge)
    //                                              .configurationHandler(modbusBridge)
    //                                              .host(deviceConfiguration.getLocalMqttUri())
    //                                              .build();
    //
    //    modbusBridge->onSensorReading([&](const std::string& reference, const std::string& value) {
    //        wolk->addSensorReading(deviceConfiguration.getKey(), reference, value);
    //        wolk->publish();
    //    });
    //
    //    modbusBridge->onActuatorStatusChange(
    //      [&](const std::string& reference) { wolk->publishActuatorStatus(deviceConfiguration.getKey(), reference);
    //      });
    //
    //    modbusBridge->onAlarmChange([&](const std::string& reference, bool active) {
    //        wolk->addAlarm(deviceConfiguration.getKey(), reference, active);
    //        wolk->publish();
    //    });
    //
    //    modbusBridge->onConfigurationChange([&]() { wolk->publishConfiguration(deviceConfiguration.getKey()); });
    //
    //    modbusBridge->onDeviceStatusChange(
    //      [&](wolkabout::DeviceStatus::Status status) { wolk->publishDeviceStatus(deviceConfiguration.getKey(),
    //      status); });
    //
    //    wolk->addDevice(*modbusBridgeDevice);
    //    wolk->connect();
    //
    //    modbusBridge->start();
    //
    //    while (true)
    //    {
    //        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    //    }

    return 0;
}
