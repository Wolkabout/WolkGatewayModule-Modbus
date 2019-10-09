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
#include "modbus/ModbusRegisterGroup.h"
#include "modbus/ModbusRegisterMapping.h"
#include "model/ActuatorTemplate.h"
#include "model/DeviceTemplate.h"
#include "model/SensorTemplate.h"
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

void makeTemplatesFromMappings(
  const std::vector<wolkabout::ModbusRegisterMapping>& modbusRegisterMappings,
  std::vector<wolkabout::SensorTemplate>& sensorTemplates, std::vector<wolkabout::ActuatorTemplate>& actuatorTemplates,
  std::vector<wolkabout::AlarmTemplate>& alarmTemplates, std::vector<wolkabout::ConfigurationTemplate>& configurationTemplates)
{
    for (const wolkabout::ModbusRegisterMapping& modbusRegisterMapping : modbusRegisterMappings)
    {
        auto mappingType = modbusRegisterMapping.getMappingType();
        auto registerType = modbusRegisterMapping.getRegisterType();

        switch (mappingType)
        {
        case wolkabout::ModbusRegisterMapping::MappingType::DEFAULT:
            switch (registerType)
            {
            case wolkabout::ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR:
            {
                actuatorTemplates.emplace_back(modbusRegisterMapping.getName(), modbusRegisterMapping.getReference(),
                                            wolkabout::DataType::NUMERIC, modbusRegisterMapping.getDescription(), 
                                            modbusRegisterMapping.getMinimum(), modbusRegisterMapping.getMaximum());
                break;
            }

            case wolkabout::ModbusRegisterMapping::RegisterType::COIL:
            {
                actuatorTemplates.emplace_back(modbusRegisterMapping.getName(), modbusRegisterMapping.getReference(),
                                            wolkabout::DataType::BOOLEAN, modbusRegisterMapping.getDescription(), 
                                            modbusRegisterMapping.getMinimum(), modbusRegisterMapping.getMaximum());
                break;
            }

            case wolkabout::ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_SENSOR:
            {
                sensorTemplates.emplace_back(modbusRegisterMapping.getName(), modbusRegisterMapping.getReference(),
                                            wolkabout::DataType::NUMERIC, modbusRegisterMapping.getDescription(),
                                            modbusRegisterMapping.getMinimum(), modbusRegisterMapping.getMaximum());
                break;
            }

            case wolkabout::ModbusRegisterMapping::RegisterType::INPUT_REGISTER:
            {
                sensorTemplates.emplace_back(modbusRegisterMapping.getName(), modbusRegisterMapping.getReference(),
                                            wolkabout::DataType::NUMERIC, modbusRegisterMapping.getDescription(),
                                            modbusRegisterMapping.getMinimum(), modbusRegisterMapping.getMaximum());
                break;
            }

            case wolkabout::ModbusRegisterMapping::RegisterType::INPUT_CONTACT:
            {
                sensorTemplates.emplace_back(modbusRegisterMapping.getName(), modbusRegisterMapping.getReference(),
                                            wolkabout::DataType::BOOLEAN, modbusRegisterMapping.getDescription(),
                                            modbusRegisterMapping.getMinimum(), modbusRegisterMapping.getMaximum());
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
        break;
            
        // Decide on the data type
        wolkabout::DataType dataType;
        if (registerType == wolkabout::ModbusRegisterMapping::RegisterType::COIL ||
            registerType == wolkabout::ModbusRegisterMapping::RegisterType::INPUT_CONTACT) {
                dataType = wolkabout::DataType::BOOLEAN;
            } else {
                dataType = wolkabout::DataType::NUMERIC;
            }

        case wolkabout::ModbusRegisterMapping::MappingType::SENSOR:
            switch (registerType)
            {
            case wolkabout::ModbusRegisterMapping::RegisterType::INPUT_CONTACT:
            case wolkabout::ModbusRegisterMapping::RegisterType::INPUT_REGISTER:
            case wolkabout::ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_SENSOR:
                sensorTemplates.emplace_back(modbusRegisterMapping.getName(), modbusRegisterMapping.getReference(),
                                            dataType, modbusRegisterMapping.getDescription(),
                                            modbusRegisterMapping.getMinimum(), modbusRegisterMapping.getMaximum());
                break;
            default:
                LOG(WARN) << "WolkGatewayModbusModule Application: Mapping with reference '"
                        << modbusRegisterMapping.getReference()
                        << "' not added to device manifest - Incompatible Mapping and Register type combination";
                break;
            }
            break;
        
        case wolkabout::ModbusRegisterMapping::MappingType::ACTUATOR:
            switch (registerType)
            {
            case wolkabout::ModbusRegisterMapping::RegisterType::COIL:
            case wolkabout::ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR:
                actuatorTemplates.emplace_back(modbusRegisterMapping.getName(), modbusRegisterMapping.getReference(),
                                            dataType, modbusRegisterMapping.getDescription(), 
                                            modbusRegisterMapping.getMinimum(), modbusRegisterMapping.getMaximum());
                break;
            default:
                LOG(WARN) << "WolkGatewayModbusModule Application: Mapping with reference '"
                        << modbusRegisterMapping.getReference()
                        << "' not added to device manifest - Incompatible Mapping and Register type combination";
                break;
            }
            break;

        case wolkabout::ModbusRegisterMapping::MappingType::ALARM:
            switch (registerType)
            {
            case wolkabout::ModbusRegisterMapping::RegisterType::INPUT_CONTACT:
                alarmTemplates.emplace_back(modbusRegisterMapping.getName(), modbusRegisterMapping.getReference(), modbusRegisterMapping.getDescription());
                break;
            default:
                LOG(WARN) << "WolkGatewayModbusModule Application: Mapping with reference '"
                        << modbusRegisterMapping.getReference()
                        << "' not added to device manifest - Incompatible Mapping and Register type combination";
                break;
            }
            break;

        case wolkabout::ModbusRegisterMapping::MappingType::CONFIGURATION:
            switch (registerType)
            {
            case wolkabout::ModbusRegisterMapping::RegisterType::COIL:
            case wolkabout::ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR:
                configurationTemplates.emplace_back(modbusRegisterMapping.getName(), modbusRegisterMapping.getReference(),
                                            dataType, modbusRegisterMapping.getDescription(), std::string(""),
                                            modbusRegisterMapping.getMinimum(), modbusRegisterMapping.getMaximum());
                break;
            default:
                LOG(WARN) << "WolkGatewayModbusModule Application: Mapping with reference '"
                        << modbusRegisterMapping.getReference()
                        << "' not added to device manifest - Incompatible Mapping and Register type combination";
                break;
            }
            break;

        default:
            break;
        }
    }
}
}    // namespace

int main(int argc, char** argv)
{
    auto logger = std::unique_ptr<wolkabout::ConsoleLogger>(new wolkabout::ConsoleLogger());
    logger->setLogLevel(wolkabout::LogLevel::TRACE);
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

    std::vector<wolkabout::SensorTemplate> sensorTemplates;
    std::vector<wolkabout::ActuatorTemplate> actuatorTemplates;
    std::vector<wolkabout::AlarmTemplate> alarmTemplates;
    std::vector<wolkabout::ConfigurationTemplate> configurationTemplates;
    makeTemplatesFromMappings(modbusRegisterMappings, sensorTemplates,
                                                             actuatorTemplates, alarmTemplates, configurationTemplates);

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
              modbusConfiguration.getResponseTimeout()));
        }

        throw std::logic_error("Unsupported Modbus implementation specified in modbus configuration file");
    }();

    auto modbusBridge = std::make_shared<wolkabout::ModbusBridge>(*libModbusClient, std::move(modbusRegisterMappings),
                                                                  modbusConfiguration.getReadPeriod());

    auto modbusBridgeTemplate = std::unique_ptr<wolkabout::DeviceTemplate>(
      new wolkabout::DeviceTemplate({configurationTemplates}, {sensorTemplates}, {alarmTemplates}, {actuatorTemplates}, "", {}, {}, {}));

    auto modbusBridgeDevice = std::make_shared<wolkabout::Device>(deviceConfiguration.getName(),
                                                                  deviceConfiguration.getKey(), *modbusBridgeTemplate);

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
