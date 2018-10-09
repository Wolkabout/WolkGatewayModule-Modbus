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
#include "modbus/LibModbusSerialRtuClient.h"
#include "modbus/LibModbusTcpIpClient.h"
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
#include <map>
#include <memory>
#include <random>
#include <string>
#include <thread>

namespace
{
bool loadDeviceConfigurationFromFile(const std::string& file, wolkabout::DeviceConfiguration& deviceConfiguration)
{
    try
    {
        deviceConfiguration = wolkabout::DeviceConfiguration::fromJsonFile(file);
        LOG(INFO) << "WolkGatewayModbusModule Application: Loaded device configuration from '" << file << "'";
        return true;
    }
    catch (std::logic_error& e)
    {
        LOG(ERROR) << "WolkGatewayModbusModule Application: Unable to parse device configuration file."
                   << "Reason: " << e.what();
        return false;
    }
}

bool loadModbusConfigurationFromFile(const std::string& file, wolkabout::ModbusConfiguration& modbusConfiguration)
{
    try
    {
        modbusConfiguration = wolkabout::ModbusConfiguration::fromJsonFile(file);
        LOG(INFO) << "WolkGatewayModbusModule Application: Loaded modbus configuration from '" << file << "'";
        return true;
    }
    catch (std::logic_error& e)
    {
        LOG(ERROR) << "WolkGatewayModbusModule Application: Unable to parse modbus configuration file."
                   << "Reason: " << e.what();
        return false;
    }
}

bool loadModbusRegisterMappingFromFile(const std::string& file,
                                       std::vector<wolkabout::ModbusRegisterMapping>& modbusRegisterMappings)
{
    try
    {
        modbusRegisterMappings = wolkabout::ModbusRegisterMappingFactory::fromJsonFile(file);
        LOG(INFO) << "WolkGatewayModbusModule Application: Loaded modbus register mapping from '" << file << "'";
        return true;
    }
    catch (std::logic_error& e)
    {
        LOG(ERROR) << "WolkGatewayModbusModule Application: Unable to parse modbus register mapping file."
                   << "Reason: " << e.what();
        return false;
    }
}

void makeSensorAndActuatorManifestsFromModbusRegisterMappings(
  const std::vector<wolkabout::ModbusRegisterMapping>& modbusRegisterMappings,
  std::vector<wolkabout::SensorManifest>& sensorManifests, std::vector<wolkabout::ActuatorManifest>& actuatorManifests)
{
    for (const wolkabout::ModbusRegisterMapping& modbusRegisterMapping : modbusRegisterMappings)
    {
        switch (modbusRegisterMapping.getRegisterType())
        {
        case wolkabout::ModbusRegisterMapping::RegisterType::HOLDING_REGISTER:
        {
            actuatorManifests.emplace_back(modbusRegisterMapping.getName(), modbusRegisterMapping.getReference(),
                                           wolkabout::DataType::NUMERIC, "",
                                           modbusRegisterMapping.getMinimum(), modbusRegisterMapping.getMaximum());
            break;
        }

        case wolkabout::ModbusRegisterMapping::RegisterType::COIL:
        {
            actuatorManifests.emplace_back(modbusRegisterMapping.getName(), modbusRegisterMapping.getReference(),
                                           wolkabout::DataType::BOOLEAN, "",
                                           modbusRegisterMapping.getMinimum(), modbusRegisterMapping.getMaximum());
            break;
        }

        case wolkabout::ModbusRegisterMapping::RegisterType::INPUT_REGISTER:
        {
            sensorManifests.emplace_back(modbusRegisterMapping.getName(), modbusRegisterMapping.getReference(),
                                         wolkabout::ReadingType::Name::GENERIC, wolkabout::ReadingType::MeasurmentUnit::COUNT,
                                         "", modbusRegisterMapping.getMinimum(), modbusRegisterMapping.getMaximum());
            break;
        }

        case wolkabout::ModbusRegisterMapping::RegisterType::INPUT_BIT:
        {
            sensorManifests.emplace_back(modbusRegisterMapping.getName(), modbusRegisterMapping.getReference(),
                                         wolkabout::ReadingType::Name::SWITCH, wolkabout::ReadingType::MeasurmentUnit::BOOLEAN,
                                         "", modbusRegisterMapping.getMinimum(), modbusRegisterMapping.getMaximum());
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
}
}    // namespace

int main(int argc, char** argv)
{
    auto logger = std::unique_ptr<wolkabout::ConsoleLogger>(new wolkabout::ConsoleLogger());
    logger->setLogLevel(wolkabout::LogLevel::INFO);
    wolkabout::Logger::setInstance(std::move(logger));

    if (argc < 4)
    {
        LOG(ERROR) << "WolkGatewayModbusModule Application: Usage -  " << argv[0]
                   << " [deviceConfigurationFilePath] [modbusConfigurationFilePath] [modbusRegisterMappingFilePath]";
        return -1;
    }

    wolkabout::DeviceConfiguration deviceConfiguration;
    if (!loadDeviceConfigurationFromFile(argv[1], deviceConfiguration))
    {
        return -1;
    }

    wolkabout::ModbusConfiguration modbusConfiguration;
    if (!loadModbusConfigurationFromFile(argv[2], modbusConfiguration))
    {
        return -1;
    }

    std::vector<wolkabout::ModbusRegisterMapping> modbusRegisterMappings;
    if (!loadModbusRegisterMappingFromFile(argv[3], modbusRegisterMappings))
    {
        return -1;
    }

    if (modbusRegisterMappings.empty())
    {
        LOG(ERROR) << "WolkGatewayModbusModule Application: Register mapping file is empty";
        return -1;
    }

    std::vector<wolkabout::SensorManifest> sensorManifests;
    std::vector<wolkabout::ActuatorManifest> actuatorManifests;
    makeSensorAndActuatorManifestsFromModbusRegisterMappings(modbusRegisterMappings, sensorManifests,
                                                             actuatorManifests);

    auto libModbusClient = [&]() -> std::unique_ptr<wolkabout::ModbusClient> {
        if (modbusConfiguration.getConnectionType() == wolkabout::ModbusConfiguration::ConnectionType::TCP_IP)
        {
            return std::unique_ptr<wolkabout::LibModbusTcpIpClient>(new wolkabout::LibModbusTcpIpClient(
              modbusConfiguration.getIp(), modbusConfiguration.getPort(), modbusConfiguration.getResponseTimeout()));
        }
        else if (modbusConfiguration.getConnectionType() == wolkabout::ModbusConfiguration::ConnectionType::SERIAL_RTU)
        {
            return std::unique_ptr<wolkabout::LibModbusSerialRtuClient>(new wolkabout::LibModbusSerialRtuClient(
              modbusConfiguration.getSerialPort(), modbusConfiguration.getBaudRate(), modbusConfiguration.getDataBits(),
              modbusConfiguration.getStopBits(), modbusConfiguration.getBitParity(),
              modbusConfiguration.getSlaveAddress(), modbusConfiguration.getResponseTimeout()));
        }

        throw std::logic_error("Unsupported Modbus implementation specified in modbus configuration file");
    }();

    auto modbusBridge = std::make_shared<wolkabout::ModbusBridge>(*libModbusClient, modbusRegisterMappings,
                                                                  modbusConfiguration.getReadPeriod());

    auto modbusBridgeManifest = std::unique_ptr<wolkabout::DeviceManifest>(new wolkabout::DeviceManifest(
      deviceConfiguration.getName() + "_Manifest", "WolkGateway Modbus Module", deviceConfiguration.getProtocol(), "",
      {}, {sensorManifests}, {}, {actuatorManifests}));

    auto modbusBridgeDevice = std::make_shared<wolkabout::Device>(deviceConfiguration.getName(),
                                                                  deviceConfiguration.getKey(), *modbusBridgeManifest);

    std::unique_ptr<wolkabout::Wolk> wolk = wolkabout::Wolk::newBuilder()
                                              .deviceStatusProvider(modbusBridge)
                                              .actuatorStatusProvider(modbusBridge)
                                              .actuationHandler(modbusBridge)
                                              .host(deviceConfiguration.getLocalMqttUri())
                                              .build();

    modbusBridge->onSensorReading([&](const std::string& reference, const std::string& value) -> void {
        wolk->addSensorReading(deviceConfiguration.getKey(), reference, value);
        wolk->publish();
    });

    modbusBridge->onActuatorStatusChange([&](const std::string& reference) -> void {
        wolk->publishActuatorStatus(deviceConfiguration.getKey(), reference);
    });

    wolk->addDevice(*modbusBridgeDevice);
    wolk->connect();

    modbusBridge->start();

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    return 0;
}
