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

#include "ModbusRegisterMapping.h"
#include "utilities/FileSystemUtils.h"
#include "utilities/json.hpp"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace wolkabout
{
using nlohmann::json;

ModbusRegisterMapping::ModbusRegisterMapping(std::string name, std::string reference, double minimum, double maximum,
                                             int address, wolkabout::ModbusRegisterMapping::RegisterType registerType,
                                             wolkabout::ModbusRegisterMapping::DataType dataType, int slaveAddress)
: m_name(std::move(name))
, m_reference(std::move(reference))
, m_minimum(minimum)
, m_maximum(maximum)
, m_address(address)
, m_registerType(registerType)
, m_dataType(dataType)
, m_slaveAddress(slaveAddress)
{
}

int ModbusRegisterMapping::rankRegisterType(ModbusRegisterMapping::RegisterType type)
{
    switch (type)
    {
        case ModbusRegisterMapping::RegisterType::COIL:
            return 1;
        case ModbusRegisterMapping::RegisterType::INPUT_BIT:
            return 2;
        case ModbusRegisterMapping::RegisterType::INPUT_REGISTER:
            return 3;
        case ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_SENSOR:
            return 4;
        case ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR:
            return 5;
    }
}

bool ModbusRegisterMapping::operator<(wolkabout::ModbusRegisterMapping::RegisterType lhs,
                                      wolkabout::ModbusRegisterMapping::RegisterType rhs)
{
    return ModbusRegisterMapping::rankRegisterType(lhs) < ModbusRegisterMapping::rankRegisterType(rhs);
}

bool ModbusRegisterMapping::operator==(wolkabout::ModbusRegisterMapping::RegisterType lhs,
                                       wolkabout::ModbusRegisterMapping::RegisterType rhs)
{
    return ModbusRegisterMapping::rankRegisterType(lhs) == ModbusRegisterMapping::rankRegisterType(rhs);
}

bool ModbusRegisterMapping::operator>(wolkabout::ModbusRegisterMapping::RegisterType lhs,
                                      wolkabout::ModbusRegisterMapping::RegisterType rhs)
{
    return ModbusRegisterMapping::rankRegisterType(lhs) > ModbusRegisterMapping::rankRegisterType(rhs);
}

bool ModbusRegisterMapping::operator!=(wolkabout::ModbusRegisterMapping::RegisterType lhs,
                                       wolkabout::ModbusRegisterMapping::RegisterType rhs)
{
    return ModbusRegisterMapping::rankRegisterType(lhs) != ModbusRegisterMapping::rankRegisterType(rhs);
}

int ModbusRegisterMapping::rankDataType(wolkabout::ModbusRegisterMapping::DataType type)
{
    switch (type)
    {
        case ModbusRegisterMapping::DataType::BOOL:
            return 1;
        case ModbusRegisterMapping::DataType::INT16:
            return 2;
        case ModbusRegisterMapping::DataType::UINT16:
            return 3;
        case ModbusRegisterMapping::DataType::REAL32:
            return 4;
    }
}

bool ModbusRegisterMapping::operator<(wolkabout::ModbusRegisterMapping::DataType lhs,
                                      wolkabout::ModbusRegisterMapping::DataType rhs)
{
    return ModbusRegisterMapping::rankDataType(lhs) < ModbusRegisterMapping::rankDataType(rhs);
}

bool ModbusRegisterMapping::operator==(wolkabout::ModbusRegisterMapping::DataType lhs,
                                       wolkabout::ModbusRegisterMapping::DataType rhs)
{
    return ModbusRegisterMapping::rankDataType(lhs) == ModbusRegisterMapping::rankDataType(rhs);
}

bool ModbusRegisterMapping::operator>(wolkabout::ModbusRegisterMapping::DataType lhs,
                                      wolkabout::ModbusRegisterMapping::DataType rhs)
{
    return ModbusRegisterMapping::rankDataType(lhs) > ModbusRegisterMapping::rankDataType(rhs);
}

bool ModbusRegisterMapping::operator!=(wolkabout::ModbusRegisterMapping::DataType lhs,
                                       wolkabout::ModbusRegisterMapping::DataType rhs)
{
    return ModbusRegisterMapping::rankDataType(lhs) != ModbusRegisterMapping::rankDataType(rhs);
}

const std::string& ModbusRegisterMapping::getName() const
{
    return m_name;
}

const std::string& ModbusRegisterMapping::getReference() const
{
    return m_reference;
}

double ModbusRegisterMapping::getMinimum() const
{
    return m_minimum;
}

double ModbusRegisterMapping::getMaximum() const
{
    return m_maximum;
}

int ModbusRegisterMapping::getAddress() const
{
    return m_address;
}

ModbusRegisterMapping::RegisterType ModbusRegisterMapping::getRegisterType() const
{
    return m_registerType;
}

ModbusRegisterMapping::DataType ModbusRegisterMapping::getDataType() const
{
    return m_dataType;
}

int ModbusRegisterMapping::getSlaveAddress() const
{
    return m_slaveAddress;
}

std::vector<wolkabout::ModbusRegisterMapping> ModbusRegisterMappingFactory::fromJsonFile(
  const std::string& modbusRegisterMappingFile)
{
    if (!FileSystemUtils::isFilePresent(modbusRegisterMappingFile))
    {
        throw std::logic_error("Given modbus configuration file does not exist.");
    }

    std::string modbusRegisterMappingJsonStr;
    if (!FileSystemUtils::readFileContent(modbusRegisterMappingFile, modbusRegisterMappingJsonStr))
    {
        throw std::logic_error("Unable to read modbus configuration file.");
    }

    std::vector<wolkabout::ModbusRegisterMapping> modbusRegisterMappingVector;

    auto j = json::parse(modbusRegisterMappingJsonStr);
    for (const auto& modbusRegisterMappingJson : j["modbusRegisterMapping"].get<json::array_t>())
    {
        const auto name = modbusRegisterMappingJson.at("name").get<std::string>();
        const auto reference = modbusRegisterMappingJson.at("reference").get<std::string>();

        const auto address = modbusRegisterMappingJson.at("address").get<int>();

        const auto registerTypeStr = modbusRegisterMappingJson.at("registerType").get<std::string>();
        const auto registerType = deserializeRegisterType(registerTypeStr);

        const auto dataTypeStr = modbusRegisterMappingJson.at("dataType").get<std::string>();
        const auto dataType = deserializeDataType(dataTypeStr);

        const auto slaveAddress = modbusRegisterMappingJson.at("slaveAddress").get<int>();

        double minimum = 0.0;
        double maximum = 1.0;

        if (dataType == ModbusRegisterMapping::DataType::INT16 || dataType == ModbusRegisterMapping::DataType::UINT16 ||
            dataType == ModbusRegisterMapping::DataType::REAL32)
        {
            minimum = modbusRegisterMappingJson.at("minimum").get<double>();
            maximum = modbusRegisterMappingJson.at("maximum").get<double>();
        }

        modbusRegisterMappingVector.emplace_back(name, reference, minimum, maximum, address, registerType, dataType,
                                                 slaveAddress);
    }

    return modbusRegisterMappingVector;
}

ModbusRegisterMapping::RegisterType ModbusRegisterMappingFactory::deserializeRegisterType(
  const std::string& registerType)
{
    if (registerType == "INPUT_REGISTER")
    {
        return ModbusRegisterMapping::RegisterType::INPUT_REGISTER;
    }
    else if (registerType == "HOLDING_REGISTER_SENSOR")
    {
        return ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_SENSOR;
    }
    else if (registerType == "HOLDING_REGISTER_ACTUATOR")
    {
        return ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR;
    }
    else if (registerType == "INPUT_BIT")
    {
        return ModbusRegisterMapping::RegisterType::INPUT_BIT;
    }
    else if (registerType == "COIL")
    {
        return ModbusRegisterMapping::RegisterType::COIL;
    }

    throw std::logic_error("Unknown modbus register type: " + registerType);
}

ModbusRegisterMapping::DataType ModbusRegisterMappingFactory::deserializeDataType(const std::string& dataType)
{
    if (dataType == "INT16")
    {
        return ModbusRegisterMapping::DataType::INT16;
    }
    else if (dataType == "UINT16")
    {
        return ModbusRegisterMapping::DataType::UINT16;
    }
    else if (dataType == "REAL32")
    {
        return ModbusRegisterMapping::DataType::REAL32;
    }
    else if (dataType == "BOOL")
    {
        return ModbusRegisterMapping::DataType::BOOL;
    }

    throw std::logic_error("Unknown data type: " + dataType);
}
}    // namespace wolkabout
