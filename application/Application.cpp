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

#include "DeviceConfiguration.h"
#include "Wolk.h"
#include "modbus/ModbusConfiguration.h"
#include "modbus/ModbusRegisterMapping.h"
#include "model/DeviceManifest.h"
#include "service/FirmwareInstaller.h"
#include "utilities/ConsoleLogger.h"

#include <chrono>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <thread>

int main(int argc, char** argv)
{
    auto logger = std::unique_ptr<wolkabout::ConsoleLogger>(new wolkabout::ConsoleLogger());
    logger->setLogLevel(wolkabout::LogLevel::DEBUG);
    wolkabout::Logger::setInstance(std::move(logger));

    if (argc < 4)
    {
        LOG(ERROR) << "WolkGatewayModbusModule Application: Usage -  " << argv[0]
                   << " [deviceConfigurationFilePath] [modbusConfigurationFilePath] [modbusRegisterMappingFilePath]";
        return -1;
    }

    wolkabout::DeviceConfiguration deviceConfiguration;
    try
    {
        deviceConfiguration = wolkabout::DeviceConfiguration::fromJson(argv[1]);
        LOG(INFO) << "WolkGatewayModbusModule Application: Loaded device configuration from '" << argv[1] << "'";
    }
    catch (std::logic_error& e)
    {
        LOG(ERROR) << "WolkGatewayModbusModule Application: Unable to parse device configuration file. Reason: "
                   << e.what();
        return -1;
    }

    wolkabout::ModbusConfiguration modbusConfiguration;
    try
    {
        modbusConfiguration = wolkabout::ModbusConfiguration::fromJson(argv[2]);
        LOG(INFO) << "WolkGatewayModbusModule Application: Loaded modbus configuration from '" << argv[2] << "'";
    }
    catch (std::logic_error& e)
    {
        LOG(ERROR) << "WolkGatewayModbusModule Application: Unable to parse modbus configuration file. Reason: "
                   << e.what();
        return -1;
    }

    std::vector<wolkabout::ModbusRegisterMapping> modbusRegisterMapping;
    try
    {
        const auto registerMapping = wolkabout::ModbusRegisterMappingFactory::fromJson(argv[3]);
        LOG(INFO) << "WolkGatewayModbusModule Application: Loaded modbus register mapping from '" << argv[3] << "'";
    }
    catch (std::logic_error& e)
    {
        LOG(ERROR) << "WolkGatewayModbusModule Application: Unable to parse modbus register mapping file. Reason: "
                   << e.what();
        return -1;
    }

    wolkabout::DeviceManifest deviceManifest1{
      "DEVICE_MANIFEST_NAME_1", "DEVICE_MANIFEST_DESCRIPTION_1", "JsonProtocol", "", {}, {}, {}, {}};
    wolkabout::Device device1{"DEVICE_NAME_1", "DEVICE_KEY_1", deviceManifest1};

    std::unique_ptr<wolkabout::Wolk> wolk =
      wolkabout::Wolk::newBuilder()
        .deviceStatusProvider([](const std::string & /* deviceKey */) -> wolkabout::DeviceStatus {
            return wolkabout::DeviceStatus::CONNECTED;
        })
        .host(deviceConfiguration.getLocalMqttUri())
        .build();

    wolk->connect();

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return 0;
}
