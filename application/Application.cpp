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
#include "modbus/LibModbusClient.h"
#include "modbus/ModbusBridge.h"
#include "modbus/ModbusClient.h"
#include "modbus/ModbusConfiguration.h"
#include "modbus/ModbusRegisterMapping.h"
#include "model/ActuatorManifest.h"
#include "model/DeviceManifest.h"
#include "model/SensorManifest.h"
#include "service/FirmwareInstaller.h"
#include "utilities/ConsoleLogger.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
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

    std::unique_ptr<std::vector<wolkabout::ModbusRegisterMapping>> modbusRegisterMappings;
    try
    {
        modbusRegisterMappings = wolkabout::ModbusRegisterMappingFactory::fromJson(argv[3]);
        LOG(INFO) << "WolkGatewayModbusModule Application: Loaded modbus register mapping from '" << argv[3] << "'";
    }
    catch (std::logic_error& e)
    {
        LOG(ERROR) << "WolkGatewayModbusModule Application: Unable to parse modbus register mapping file. Reason: "
                   << e.what();
        return -1;
    }

    if (modbusRegisterMappings->empty())
    {
        LOG(ERROR) << "WolkGatewayModbusModule Application: Register mapping file is empty";
        return -1;
    }

    auto sensorManifests =
      std::unique_ptr<std::vector<wolkabout::SensorManifest>>(new std::vector<wolkabout::SensorManifest>());
    auto actuatorManifests =
      std::unique_ptr<std::vector<wolkabout::ActuatorManifest>>(new std::vector<wolkabout::ActuatorManifest>());

    // TODO: Extract to function
    for (const wolkabout::ModbusRegisterMapping& modbusRegisterMapping : *modbusRegisterMappings)
    {
        switch (modbusRegisterMapping.getRegisterType())
        {
        case wolkabout::ModbusRegisterMapping::RegisterType::HOLDING_REGISTER:
        {
            double minimum = 0.0;
            double maximum = 0.0;
            int precision = 0;

            if (modbusRegisterMapping.getDataType() == wolkabout::ModbusRegisterMapping::DataType::INT16)
            {
                minimum = -32768.0;
                maximum = 32767.0;
                precision = 5;
            }
            else if (modbusRegisterMapping.getDataType() == wolkabout::ModbusRegisterMapping::DataType::UINT16)
            {
                minimum = 0.0;
                maximum = 65535.0;
                precision = 5;
            }
            else if (modbusRegisterMapping.getDataType() == wolkabout::ModbusRegisterMapping::DataType::REAL32)
            {
                minimum = std::numeric_limits<float>::min();
                maximum = std::numeric_limits<float>::max();
                precision = std::numeric_limits<float>::max_digits10;
            }

            actuatorManifests->emplace_back(
              modbusRegisterMapping.getName(), modbusRegisterMapping.getReference(), "description", "unit", "GENERIC",
              wolkabout::ActuatorManifest::DataType::NUMERIC, precision, minimum, maximum);
            break;
        }

        case wolkabout::ModbusRegisterMapping::RegisterType::COIL:
        {
            actuatorManifests->emplace_back(modbusRegisterMapping.getName(), modbusRegisterMapping.getReference(),
                                            "description", "unit", "GENERIC",
                                            wolkabout::ActuatorManifest::DataType::BOOLEAN, 1, 0, 1);
            break;
        }

        case wolkabout::ModbusRegisterMapping::RegisterType::INPUT_REGISTER:
        {
            double minimum = 0.0;
            double maximum = 0.0;
            int precision = 0;

            if (modbusRegisterMapping.getDataType() == wolkabout::ModbusRegisterMapping::DataType::INT16)
            {
                minimum = -32768.0;
                maximum = 32767.0;
                precision = 5;
            }
            else if (modbusRegisterMapping.getDataType() == wolkabout::ModbusRegisterMapping::DataType::UINT16)
            {
                minimum = 0.0;
                maximum = 65535.0;
                precision = 5;
            }
            else if (modbusRegisterMapping.getDataType() == wolkabout::ModbusRegisterMapping::DataType::REAL32)
            {
                minimum = std::numeric_limits<float>::min();
                maximum = std::numeric_limits<float>::max();
                precision = std::numeric_limits<float>::max_digits10 + std::numeric_limits<float>::max_exponent10;
            }

            sensorManifests->emplace_back(modbusRegisterMapping.getName(), modbusRegisterMapping.getReference(),
                                          "description", "unit", "GENERIC",
                                          wolkabout::SensorManifest::DataType::NUMERIC, precision, minimum, maximum);
            break;
        }

        case wolkabout::ModbusRegisterMapping::RegisterType::INPUT_BIT:
        {
            sensorManifests->emplace_back(modbusRegisterMapping.getName(), modbusRegisterMapping.getReference(),
                                          "description", "unit", "GENERIC",
                                          wolkabout::SensorManifest::DataType::BOOLEAN, 1, 0, 1);
            break;
        }

        default:
        {
            LOG(WARN) << "WolkGatewayModbusModule Application: Mapping with reference '"
                      << modbusRegisterMapping.getReference()
                      << "' not added to device manifest - Unknown register type";
            break;
        }
        }
    }

    auto libModbusClient = std::unique_ptr<wolkabout::LibModbusClient>(new wolkabout::LibModbusClient(
      modbusConfiguration.getIp(), modbusConfiguration.getPort(), modbusConfiguration.getResponseTimeout()));

    auto modbusBridge = std::make_shared<wolkabout::ModbusBridge>(*libModbusClient, *modbusRegisterMappings,
                                                                  modbusConfiguration.getReadPeriod());

    auto modbusBridgeManifest = std::unique_ptr<wolkabout::DeviceManifest>(new wolkabout::DeviceManifest(
      deviceConfiguration.getName() + "_Manifest", "WolkGateway Modbus Module", deviceConfiguration.getProtocol(), "",
      {}, {*sensorManifests}, {}, {*actuatorManifests}));

    auto modbusBridgeDevice = std::make_shared<wolkabout::Device>(deviceConfiguration.getName(),
                                                                  deviceConfiguration.getKey(), *modbusBridgeManifest);

    std::unique_ptr<wolkabout::Wolk> wolk = wolkabout::Wolk::newBuilder()
                                              .deviceStatusProvider(modbusBridge)
                                              .actuatorStatusProvider(modbusBridge)
                                              .actuationHandler(modbusBridge)
                                              .host(deviceConfiguration.getLocalMqttUri())
                                              .build();

    wolk->addDevice(*modbusBridgeDevice);
    wolk->connect();

    modbusBridge->start();

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
