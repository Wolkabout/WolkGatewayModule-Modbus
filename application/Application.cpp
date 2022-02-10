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

#include "core/utilities/Logger.h"
#include "modbus/model/DevicesConfiguration.h"
#include "modbus/model/ModuleConfiguration.h"
#include "modbus/module/ModbusBridge.h"
#include "modbus/module/WolkaboutTemplateFactory.h"
#include "modbus/module/persistence/JsonFilePersistence.h"
#include "modbus/utilities/JsonReaderParser.h"
#include "more_modbus/mappings/StringMapping.h"
#include "more_modbus/modbus/LibModbusSerialRtuClient.h"
#include "more_modbus/modbus/LibModbusTcpIpClient.h"
#include "wolk/WolkMulti.h"

#include <algorithm>
#include <chrono>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <utility>

using namespace wolkabout;
using namespace wolkabout::connect;
using namespace wolkabout::more_modbus;
using namespace wolkabout::modbus;

const std::string DEFAULT_VALUE_PERSISTENCE_FILE = "./default-values.json";
const std::string REPEATED_WRITE_PERSISTENCE_FILE = "./repeat-write.json";
const std::string SAFE_MODE_WRITE_PERSISTENCE_FILE = "./safe-mode.json";

using RegistrationDataMap = std::map<std::string, std::unique_ptr<DeviceRegistrationData>>;
using DeviceMap = std::map<std::uint16_t, std::unique_ptr<Device>>;
using DeviceTypeMap = std::map<std::string, std::vector<std::uint16_t>>;

RegistrationDataMap generateRegistrationData(const DevicesConfiguration& devicesConfiguration)
{
    auto templates = RegistrationDataMap{};
    for (const auto& deviceTemplate : devicesConfiguration.getTemplates())
        templates.emplace(deviceTemplate.first, WolkaboutTemplateFactory::makeRegistrationDataFromDeviceConfigTemplate(
                                                  *deviceTemplate.second));
    return templates;
}

std::pair<DeviceMap, DeviceTypeMap> generateDevices(const ModuleConfiguration& moduleConfiguration,
                                                    const DevicesConfiguration& devicesConfiguration,
                                                    const RegistrationDataMap& deviceRegistrationData)
{
    // Make place for the values we are going to return.
    auto deviceMap = DeviceMap{};
    auto deviceTypeMap = DeviceTypeMap{};

    // Parse devices with templates to Device
    // We need to check, in the SERIAL/RTU, that all devices have a slave address
    // and that they're different from one another. In TCP/IP mode, we can have only
    // one device, so we need to check for that (and assign it a slaveAddress, because -1 is an invalid address).
    auto assigningSlaveAddress = std::uint16_t{1};

    // Go through the devices from the config
    for (const auto& deviceInformation : devicesConfiguration.getDevices())
    {
        // Check the slave address
        auto& info = *deviceInformation.second;
        if (moduleConfiguration.getConnectionType() == ModuleConfiguration::ConnectionType::SERIAL_RTU)
        {
            // If it doesn't at all have a slaveAddress or if the slaveAddress is already occupied
            // device is not valid.
            if (info.getSlaveAddress() == 0)
            {
                LOG(WARN) << "Device " << info.getName() << " is missing a slave adress. Ignoring device...";
                continue;
            }
            const auto deviceIt = deviceMap.find(info.getSlaveAddress());
            if (deviceIt != deviceMap.cend())
            {
                LOG(WARN) << "Device " << info.getName() << " has a conflicting slave adress. Ignoring device...";
            }
        }
        else
        {
            // If it's an TCP/IP device, assign it the next free slaveAddress.
            info.setSlaveAddress(assigningSlaveAddress++);
        }

        // Assign the DeviceRegistrationData
        const auto& templateName = info.getTemplateString();
        const auto& pair = deviceRegistrationData.find(templateName);
        if (pair != deviceRegistrationData.end())
        {
            // Create the device with the registration data, push the slave address as occupied
            auto registrationData = *pair->second;
            deviceMap.emplace(info.getSlaveAddress(), std::unique_ptr<Device>{new Device{
                                                        info.getKey(), "", OutboundDataMode::PUSH, info.getName()}});

            // Emplace the template name in usedTemplates array for modbusBridge, and the slaveAddress
            if (deviceTypeMap.find(templateName) != deviceTypeMap.end())
                deviceTypeMap[templateName].emplace_back(info.getSlaveAddress());
            else
                deviceTypeMap.emplace(templateName, std::vector<std::uint16_t>{info.getSlaveAddress()});
        }
        else
        {
            LOG(WARN) << "Device " << info.getName() << " doesn't have a valid template. Ignoring device...";
        }
    }

    // Now return the maps
    return std::make_pair(deviceMap, deviceTypeMap);
}

int main(int argc, char** argv)
{
    // Setup logger
    Logger::init(LogLevel::TRACE, Logger::Type::CONSOLE);
    if (argc < 3)
    {
        LOG(ERROR) << "WolkGatewayModbusModule Application: Usage -  " << argv[0]
                   << " [moduleConfigurationFilePath] [devicesConfigurationFilePath]";
        return 1;
    }

    // Parse file passed in first arg - module configuration JSON file
    auto moduleConfiguration = ModuleConfiguration{JsonReaderParser::readFile(argv[1])};

    // Parse file passed in second arg - devices configuration JSON file
    auto devicesConfiguration = DevicesConfiguration{JsonReaderParser::readFile(argv[2])};

    // We need to do some checks to see if the inputted data is valid.
    // We don't want there to be no templates.
    if (devicesConfiguration.getTemplates().empty())
    {
        LOG(ERROR) << "You have not created any templates.";
        return 1;
    }

    // We don't want there to be no devices.
    if (devicesConfiguration.getDevices().empty())
    {
        LOG(ERROR) << "You have not created any devices.";
        return 1;
    }

    // Cut the execution right away if the user wants multiple TCP/IP devices.
    if (moduleConfiguration.getConnectionType() == ModuleConfiguration::ConnectionType::TCP_IP &&
        devicesConfiguration.getDevices().size() != 1)
    {
        LOG(ERROR) << "Application supports exactly one device in TCP/IP mode.";
        return 1;
    }

    // Warn the user if they're using more templates in TCP/IP.
    // Since TCP/IP supports only one device, you should have only one template.
    if (moduleConfiguration.getConnectionType() == ModuleConfiguration::ConnectionType::TCP_IP &&
        devicesConfiguration.getTemplates().size() != 1)
    {
        LOG(WARN) << "Using more than 1 template in TCP/IP mode is unnecessary. There can only be 1 TCP/IP device "
                  << "per module, which can use only one template.";
    }

    // Create the modbus client based on parsed information
    // Pass configuration parameters necessary to initialize the connection
    // according to the type of connection that the user required and setup.
    auto libModbusClient = [&]() -> std::shared_ptr<ModbusClient> {
        if (moduleConfiguration.getConnectionType() == ModuleConfiguration::ConnectionType::TCP_IP)
        {
            const auto& tcpConfiguration = moduleConfiguration.getTcpIpConfiguration();
            return std::make_shared<LibModbusTcpIpClient>(tcpConfiguration->getIp(), tcpConfiguration->getPort(),
                                                          moduleConfiguration.getResponseTimeout());
        }
        else if (moduleConfiguration.getConnectionType() == ModuleConfiguration::ConnectionType::SERIAL_RTU)
        {
            const auto& serialConfiguration = moduleConfiguration.getSerialRtuConfiguration();
            return std::make_shared<LibModbusSerialRtuClient>(
              serialConfiguration->getSerialPort(), serialConfiguration->getBaudRate(),
              serialConfiguration->getDataBits(), serialConfiguration->getStopBits(),
              serialConfiguration->getBitParity(), moduleConfiguration.getResponseTimeout());
        }

        throw std::logic_error("Unsupported Modbus implementation specified in module configuration file");
    }();

    auto registrationData = generateRegistrationData(devicesConfiguration);

    // Execute linking logic
    auto deviceMap = DeviceMap{};
    auto deviceTypeMap = DeviceTypeMap{};
    std::tie(deviceMap, deviceTypeMap) = generateDevices(moduleConfiguration, devicesConfiguration, registrationData);

    // If no devices are valid, the application has no reason to work
    if (deviceMap.empty())
    {
        LOG(ERROR) << "No devices are valid. Quitting application...";
        return 1;
    }

    // Report the device count to the user
    LOG(INFO) << "Created " << deviceMap.size() << " device(s)!";
    auto invalidDevices = devicesConfiguration.getDevices().size() - deviceMap.size();
    if (invalidDevices > 0)
    {
        LOG(WARN) << "There were " << invalidDevices << " invalid device(s)!";
    }

    // Pass everything necessary to initialize the bridge
    LOG(DEBUG) << "Initializing the bridge...";
    auto modbusBridge = std::make_shared<ModbusBridge>(
      libModbusClient, moduleConfiguration.getRegisterReadPeriod(),
      std::unique_ptr<JsonFilePersistence>{new JsonFilePersistence(DEFAULT_VALUE_PERSISTENCE_FILE)},
      std::unique_ptr<JsonFilePersistence>{new JsonFilePersistence(REPEATED_WRITE_PERSISTENCE_FILE)},
      std::unique_ptr<JsonFilePersistence>{new JsonFilePersistence(SAFE_MODE_WRITE_PERSISTENCE_FILE)});
    modbusBridge->initialize(devicesConfiguration.getTemplates(), deviceTypeMap, deviceMap);

    // Connect the bridge to Wolk instance
    LOG(DEBUG) << "Connecting with Wolk...";
    auto wolk = WolkMulti::newBuilder()
                  .host(moduleConfiguration.getMqttHost())
                  .feedUpdateHandler(modbusBridge)
                  .parameterHandler(modbusBridge)
                  .withPlatformStatus(modbusBridge)
                  .withRegistration()
                  .buildWolkMulti();

    // Setup all the necessary callbacks for value changes from inside the modbusBridge
    modbusBridge->setFeedValueCallback([&](const std::string& deviceKey, const Reading& reading) {
        wolk->addReading(deviceKey, reading);
        wolk->publish();
    });
    modbusBridge->setAttributeCallback([&](const std::string& deviceKey, const Attribute& attribute) {
        wolk->addAttribute(deviceKey, attribute);
        wolk->publish();
    });

    // Track if we registered or not
    auto connected = false;
    auto registered = false;
    std::mutex mutex;
    std::condition_variable variable;
    std::unique_lock<std::mutex> lock{mutex};

    // Set up the connection status listener to trigger states
    // TODO
    wolk->setConnectionStatusListener([&](bool connected) {

    });
    wolk->connect();

    // Register all the devices created
    for (const auto& device : deviceMap)
        wolk->registerDevices(*device.second);

    modbusBridge->start();
    for (const auto& device : devices)
    {
        wolk->publishConfiguration(device.second->getKey());
    }

    while (modbusBridge->isRunning())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
