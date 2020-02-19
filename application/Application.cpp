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
#include <iostream>
#include <map>
#include <memory>
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
    auto logger = std::unique_ptr<wolkabout::ConsoleLogger>(new wolkabout::ConsoleLogger());
    logger->setLogLevel(wolkabout::LogLevel::INFO);
    wolkabout::Logger::setInstance(std::move(logger));

    if (argc < 3)
    {
        LOG(ERROR) << "WolkGatewayModbusModule Application: Usage -  " << argv[0]
                   << " [moduleConfigurationPath] [devicesConfigurationFilePath]";
        return -1;
    }

    wolkabout::ModuleConfiguration moduleConfiguration;
    if (!loadModuleConfiguration(argv[1], moduleConfiguration))
    {
        return -1;
    }

    wolkabout::DevicesConfiguration devicesConfiguration;
    if (!loadDevicesConfiguration(argv[2], devicesConfiguration))
    {
        return -1;
    }

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

    //    std::vector<wolkabout::SensorTemplate> sensorTemplates;
    //    std::vector<wolkabout::ActuatorTemplate> actuatorTemplates;
    //    std::vector<wolkabout::AlarmTemplate> alarmTemplates;
    //    std::vector<wolkabout::ConfigurationTemplate> configurationTemplates;
    //    makeTemplatesFromMappings(modbusRegisterMappings, sensorTemplates, actuatorTemplates, alarmTemplates,
    //                              configurationTemplates);
    //
    //    auto modbusBridge = std::make_shared<wolkabout::ModbusBridge>(*libModbusClient,
    //    std::move(modbusRegisterMappings),
    //                                                                  modbusConfiguration.getReadPeriod());
    //
    //    auto modbusBridgeTemplate = std::unique_ptr<wolkabout::DeviceTemplate>(new wolkabout::DeviceTemplate(
    //      {configurationTemplates}, {sensorTemplates}, {alarmTemplates}, {actuatorTemplates}, "", {}, {}, {}));
    //
    //    auto modbusBridgeDevice = std::make_shared<wolkabout::Device>(deviceConfiguration.getName(),
    //                                                                  deviceConfiguration.getKey(),
    //                                                                  *modbusBridgeTemplate);
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
