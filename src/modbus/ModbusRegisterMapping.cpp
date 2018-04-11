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

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace wolkabout
{
using nlohmann::json;

ModbusRegisterMapping::ModbusRegisterMapping(std::string name, std::string reference, int address,
                                             wolkabout::ModbusRegisterMapping::RegisterType registerType,
                                             wolkabout::ModbusRegisterMapping::DataType dataType)
: m_name(std::move(name))
, m_reference(std::move(reference))
, m_address(address)
, m_registerType(registerType)
, m_dataType(dataType)
{
}

const std::string& ModbusRegisterMapping::getName() const
{
    return m_name;
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

const std::string& ModbusRegisterMapping::getReference() const
{
    return m_reference;
}

std::unique_ptr<std::vector<wolkabout::ModbusRegisterMapping>> ModbusRegisterMappingFactory::fromJson(
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

    auto modbusRegisterMappingVector =
      std::unique_ptr<std::vector<wolkabout::ModbusRegisterMapping>>(new std::vector<ModbusRegisterMapping>());

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

        modbusRegisterMappingVector->emplace_back(name, reference, address, registerType, dataType);
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
    else if (registerType == "HOLDING_REGISTER")
    {
        return ModbusRegisterMapping::RegisterType::HOLDING_REGISTER;
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
