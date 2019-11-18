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

#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace wolkabout
{
using nlohmann::json;

ModbusRegisterMapping::ModbusRegisterMapping(
  std::string name, std::string reference, std::string description, double minimum, double maximum, int address,
  wolkabout::ModbusRegisterMapping::RegisterType registerType, wolkabout::ModbusRegisterMapping::DataType dataType,
  int slaveAddress, wolkabout::ModbusRegisterMapping::MappingType mappingType = MappingType::DEFAULT)
: m_name(std::move(name))
, m_reference(std::move(reference))
, m_description(std::move(description))
, m_minimum(minimum)
, m_maximum(maximum)
, m_address(address)
, m_labelsAndAddresses(nullptr)
, m_registerType(registerType)
, m_dataType(dataType)
, m_mappingType(mappingType)
, m_slaveAddress(slaveAddress)
{
}

ModbusRegisterMapping::ModbusRegisterMapping(std::string name, std::string reference, std::string description,
                                             double minimum, double maximum,
                                             std::shared_ptr<LabelMap> labelsAndAddresses,
                                             ModbusRegisterMapping::RegisterType registerType,
                                             ModbusRegisterMapping::DataType dataType, int slaveAddress)
: m_name(std::move(name))
, m_reference(std::move(reference))
, m_description(std::move(description))
, m_minimum(minimum)
, m_maximum(maximum)
, m_address(-1)
, m_labelsAndAddresses(std::move(labelsAndAddresses))
, m_registerType(registerType)
, m_dataType(dataType)
, m_mappingType(MappingType::CONFIGURATION)
, m_slaveAddress(slaveAddress)
{
}

const std::string& ModbusRegisterMapping::getName() const
{
    return m_name;
}

const std::string& ModbusRegisterMapping::getReference() const
{
    return m_reference;
}

const std::string& ModbusRegisterMapping::getDescription() const
{
    return m_description;
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
    if (m_address == -1)
    {
        int min = m_labelsAndAddresses->begin()->second;
        for (auto& label : *m_labelsAndAddresses)
        {
            if (label.second < min)
            {
                min = label.second;
            }
        }
        return min;
    }
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

ModbusRegisterMapping::MappingType ModbusRegisterMapping::getMappingType() const
{
    return m_mappingType;
}

std::shared_ptr<LabelMap> ModbusRegisterMapping::getLabelsAndAddresses() const
{
    return m_labelsAndAddresses;
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
        const auto registerType =
          deserializeRegisterType(modbusRegisterMappingJson.at("registerType").get<std::string>());
        const auto slaveAddress = modbusRegisterMappingJson.at("slaveAddress").get<int>();

        // description is optional
        auto description = std::string("");
        try
        {
            description = modbusRegisterMappingJson.at("description").get<std::string>();
        }
        catch (std::exception&)
        {
        }    // it will be empty

        auto mappingType = ModbusRegisterMapping::MappingType::DEFAULT;
        try
        {
            const auto mappingTypeStr = modbusRegisterMappingJson.at("mappingType").get<std::string>();
            mappingType = deserializeMappingType(mappingTypeStr);
        }
        catch (std::exception&)
        {
        }    // it will be default

        int address = -1;
        std::shared_ptr<LabelMap> labelMap;
        ModbusRegisterMapping::DataType dataType;
        double minimum = 0.0;
        double maximum = 1.0;

        if (registerType == ModbusRegisterMapping::RegisterType::COIL ||
            registerType == ModbusRegisterMapping::RegisterType::INPUT_CONTACT)
        {
            // this is obligatory here, we don't support multi-value boolean configurations
            address = modbusRegisterMappingJson.at("address").get<int>();
            dataType = ModbusRegisterMapping::DataType::BOOL;
            // we ignore dataType and min/max
        }
        else
        {
            dataType = deserializeDataType(modbusRegisterMappingJson.at("dataType").get<std::string>());
            minimum = modbusRegisterMappingJson.at("minimum").get<double>();
            maximum = modbusRegisterMappingJson.at("maximum").get<double>();
            if (registerType == ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR)
            {
                if (mappingType == ModbusRegisterMapping::MappingType::CONFIGURATION)
                {
                    // or LabelMap or address
                    // if both, make an error, or if none, make an error
                    bool gotAddress = false, gotLabelMap = false;
                    try
                    {
                        address = modbusRegisterMappingJson.at("address").get<int>();
                        gotAddress = true;
                    }
                    catch (std::exception&)
                    {
                    }
                    try
                    {
                        labelMap = std::make_shared<LabelMap>(modbusRegisterMappingJson.at("labelMap").get<LabelMap>());
                        gotLabelMap = true;
                    }
                    catch (std::exception&)
                    {
                    }
                    if (gotAddress == gotLabelMap)
                    {
                        if (gotAddress)
                        {
                            throw std::logic_error("You cannot set both address and a labelMap for " + name +
                                                   " !"
                                                   " Choose only address to make configuration single-value, or "
                                                   "labelMap to make configuration multi-value!");
                        }
                        else
                        {
                            throw std::logic_error("You have to put either address or labelMap for " + name +
                                                   " !"
                                                   " Choose either address to make configuration single-value, or"
                                                   "labelMap to make configuration multi-value!");
                        }
                    }
                }
                else
                {
                    // if it's not a configuration, we need the address
                    address = modbusRegisterMappingJson.at("address").get<int>();
                }
            }
            else
            {
                address = modbusRegisterMappingJson.at("address").get<int>();
            }
        }

        if (labelMap == nullptr)
        {
            modbusRegisterMappingVector.emplace_back(name, reference, description, minimum, maximum, address,
                                                     registerType, dataType, slaveAddress, mappingType);
        }
        else
        {
            modbusRegisterMappingVector.emplace_back(name, reference, description, minimum, maximum,
                                                     std::move(labelMap), registerType, dataType, slaveAddress);
        }
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
    else if (registerType == "INPUT_CONTACT")
    {
        return ModbusRegisterMapping::RegisterType::INPUT_CONTACT;
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

ModbusRegisterMapping::MappingType ModbusRegisterMappingFactory::deserializeMappingType(const std::string& mappingType)
{
    if (mappingType == "DEFAULT")
    {
        return ModbusRegisterMapping::MappingType::DEFAULT;
    }
    else if (mappingType == "SENSOR")
    {
        return ModbusRegisterMapping::MappingType::SENSOR;
    }
    else if (mappingType == "ACTUATOR")
    {
        return ModbusRegisterMapping::MappingType::ACTUATOR;
    }
    else if (mappingType == "ALARM")
    {
        return ModbusRegisterMapping::MappingType::ALARM;
    }
    else if (mappingType == "CONFIGURATION")
    {
        return ModbusRegisterMapping::MappingType::CONFIGURATION;
    }
    throw std::logic_error("Unknown data type: " + mappingType);
}

}    // namespace wolkabout
