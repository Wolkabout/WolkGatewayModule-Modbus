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
#include "wolk/api/PlatformStatusListener.h"

#include <algorithm>
#include <chrono>
#include <map>
#include <memory>
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

namespace
{
const auto LOG_FILE = "/var/log/modbusModule/wolkgatewaymodule-modbus.log";
}

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

    // Go through the devices from the config
    for (const auto& deviceInformation : devicesConfiguration.getDevices())
    {
        // Check the slave address
        auto& info = *deviceInformation.second;
        // If it doesn't at all have a slaveAddress or if the slaveAddress is already occupied
        // device is not valid.
        if (info.getSlaveAddress() == 0)
        {
            LOG(WARN) << "Device " << info.getName() << " is missing a slave address. Ignoring device...";
            continue;
        }
        const auto deviceIt = deviceMap.find(info.getSlaveAddress());
        if (deviceIt != deviceMap.cend())
        {
            LOG(WARN) << "Device " << info.getName() << " has a conflicting slave address. Ignoring device...";
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
    return std::make_pair(std::move(deviceMap), std::move(deviceTypeMap));
}

class StateHandler : public PlatformStatusListener
{
public:
    explicit StateHandler(ModbusBridge& modbusBridge)
    : m_modbusBridge(modbusBridge), m_connected{false}, m_registered{false}
    {
    }

    void platformStatus(ConnectivityStatus status) override
    {
        changeConnected(status == wolkabout::ConnectivityStatus::CONNECTED);
        m_modbusBridge.platformStatus(status);
        if (m_platformStatusCallback)
            m_platformStatusCallback(status == wolkabout::ConnectivityStatus::CONNECTED);
    }

    void changeConnected(bool connected)
    {
        m_connected = connected;
        if (!connected)
            m_modbusBridge.stop();
        else if (connected)
            if (m_registered)
                m_modbusBridge.start();
    }

    void changeRegistered(bool registered)
    {
        m_registered = registered;
        if (m_connected && registered)
            m_modbusBridge.start();
    }

    bool isConnected() const { return m_connected; }

    bool isRegistered() const { return m_registered; }

    void setPlatformStatusCallback(const std::function<void(bool)>& platformStatusCallback)
    {
        m_platformStatusCallback = platformStatusCallback;
    }

private:
    ModbusBridge& m_modbusBridge;
    std::function<void(bool)> m_platformStatusCallback;

    bool m_connected;
    bool m_registered;
};

wolkabout::legacy::LogLevel parseLogLevel(const std::string& levelStr)
{
    const std::string str = wolkabout::legacy::StringUtils::toUpperCase(levelStr);
    const auto logLevel = [&]() -> wolkabout::legacy::LogLevel {
        if (str == "TRACE")
            return wolkabout::legacy::LogLevel::TRACE;
        else if (str == "DEBUG")
            return wolkabout::legacy::LogLevel::DEBUG;
        else if (str == "INFO")
            return wolkabout::legacy::LogLevel::INFO;
        else if (str == "WARN")
            return wolkabout::legacy::LogLevel::WARN;
        else if (str == "ERROR")
            return wolkabout::legacy::LogLevel::ERROR;

        throw std::logic_error("Unable to parse log level.");
    }();

    return logLevel;
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        LOG(ERROR) << "WolkGatewayModbusModule Application: Usage -  " << argv[0]
                   << " [moduleConfigurationFilePath] [devicesConfigurationFilePath] [logLevel]";
        return 1;
    }

    // Setup logger
    const auto level = [&] {
        if (argc > 3)
        {
            const std::string logLevelStr{argv[3]};
            try
            {
                return parseLogLevel(logLevelStr);
            }
            catch (std::logic_error& e)
            {
                return LogLevel::INFO;
            }
        }
        return LogLevel::INFO;
    }();
    Logger::init(level, Logger::Type::CONSOLE | Logger::Type::FILE, LOG_FILE);

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
    auto stateHandler = std::make_shared<StateHandler>(*modbusBridge);
    modbusBridge->initialize(devicesConfiguration.getTemplates(), deviceTypeMap, deviceMap);

    // Connect the bridge to Wolk instance
    LOG(DEBUG) << "Connecting with Wolk...";
    auto wolk = WolkMulti::newBuilder()
                  .host(moduleConfiguration.getMqttHost())
                  .feedUpdateHandler(modbusBridge)
                  .parameterHandler(modbusBridge)
                  .withPlatformStatus(stateHandler)
                  .withRegistration()
                  .buildWolkMulti();

    // Setup all the necessary callbacks for value changes from inside the modbusBridge
    modbusBridge->setFeedValueCallback([&](const std::string& deviceKey, const std::vector<Reading>& readings) {
        wolk->addReadings(deviceKey, readings);
    });
    modbusBridge->setAttributeCallback(
      [&](const std::string& deviceKey, const Attribute& attribute) { wolk->addAttribute(deviceKey, attribute); });

    // Track if we registered or not
    auto connected = false;
    auto registered = false;
    std::mutex mutex;
    std::condition_variable variable;
    std::unique_lock<std::mutex> lock{mutex};

    // Set up the connection status listener to trigger states
    auto devicesToRegister = std::vector<DeviceRegistrationData>{};
    for (const auto& devicesPerTemplate : deviceTypeMap)
    {
        // Obtain the reference to the device registration data for this type
        const auto templateIt = registrationData.find(devicesPerTemplate.first);
        if (templateIt == registrationData.cend())
            continue;
        const auto& registrationDataOriginal = *templateIt->second;

        // Now for every device copy the data and emplace it in the vector
        for (const auto& device : devicesPerTemplate.second)
        {
            // Obtain the device information
            const auto deviceIt = deviceMap.find(device);
            if (deviceIt == deviceMap.cend())
                continue;
            const auto& deviceInfo = *deviceIt->second;

            // Make the copy and emplace
            auto copy = DeviceRegistrationData{registrationDataOriginal};
            copy.name = deviceInfo.getName();
            copy.key = deviceInfo.getKey();
            devicesToRegister.emplace_back(std::move(copy));
        }
    }

    // Make the callback for registration
    auto callbackForRegistration =
      std::function<void(const std::vector<std::string>&, const std::vector<std::string>&)>{};
    callbackForRegistration = [&](const std::vector<std::string>& registeredDevices, const std::vector<std::string>&) {
        LOG(INFO) << "Required count of devices: " << devicesToRegister.size() << ".";
        LOG(INFO) << "Registered devices: " << registeredDevices.size() << ".";
        if (registeredDevices.size() == devicesToRegister.size())
            stateHandler->changeRegistered(true);
        else
        {
            LOG(ERROR) << "Failed registration of devices. Waiting 10s...";
            std::this_thread::sleep_for(std::chrono::seconds{10});
            wolk->registerDevices(devicesToRegister, callbackForRegistration);
        }
    };

    // Now make the loop that will on connect, trigger register, and stuff like that...
    wolk->setConnectionStatusListener([&](bool newConnectionState) {
        stateHandler->changeConnected(newConnectionState);
        if (!stateHandler->isRegistered())
        {
            wolk->registerDevices(devicesToRegister, callbackForRegistration);
        }
    });

    // Also, if the registration needs to be triggered by the platform status
    stateHandler->setPlatformStatusCallback([&](bool status) {
        if (status)
            wolk->registerDevices(devicesToRegister, callbackForRegistration);
    });

    wolk->connect();
    while (modbusBridge != nullptr)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (stateHandler->isConnected() && stateHandler->isRegistered())
            wolk->publish();
    }

    return 0;
}
