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

#include <set>
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
  wolkabout::ModbusRegisterMapping::MappingType mappingType = MappingType::DEFAULT)
: m_name(std::move(name))
, m_reference(std::move(reference))
, m_description(std::move(description))
, m_minimum(minimum)
, m_maximum(maximum)
, m_address(address)
, m_labelsAndAddresses()
, m_registerType(registerType)
, m_dataType(dataType)
, m_mappingType(mappingType)
, m_isInitialized(false)
, m_isValid(true)
{
}

ModbusRegisterMapping::ModbusRegisterMapping(std::string name, std::string reference, std::string description,
                                             double minimum, double maximum, const LabelMap& labelsAndAddresses,
                                             ModbusRegisterMapping::RegisterType registerType,
                                             ModbusRegisterMapping::DataType dataType)
: m_name(std::move(name))
, m_reference(std::move(reference))
, m_description(std::move(description))
, m_minimum(minimum)
, m_maximum(maximum)
, m_address(-1)
, m_labelsAndAddresses(labelsAndAddresses)
, m_registerType(registerType)
, m_dataType(dataType)
, m_mappingType(MappingType::CONFIGURATION)
, m_isInitialized(false)
, m_isValid(true)
{
}

ModbusRegisterMapping::ModbusRegisterMapping(nlohmann::json j) : m_isInitialized(false), m_isValid(true)
{
    m_name = j.at("name").get<std::string>();
    m_reference = j.at("reference").get<std::string>();
    m_registerType = MappingTypeConversion::deserializeRegisterType(j.at("registerType").get<std::string>());

    try
    {
        m_description = j.at("description").get<std::string>();
    }
    catch (std::exception&)
    {
        m_description = std::string("");
    }

    try
    {
        const auto mappingTypeStr = j.at("mappingType").get<std::string>();
        m_mappingType = MappingTypeConversion::deserializeMappingType(mappingTypeStr);
    }
    catch (std::exception&)
    {
        m_mappingType = ModbusRegisterMapping::MappingType::DEFAULT;
    }

    if (m_registerType == ModbusRegisterMapping::RegisterType::COIL ||
        m_registerType == ModbusRegisterMapping::RegisterType::INPUT_CONTACT)
    {
        // this is obligatory here, we don't support multi-value boolean configurations
        m_address = j.at("address").get<int>();
        m_dataType = ModbusRegisterMapping::DataType::BOOL;
        // we ignore dataType and min/max
    }
    else
    {
        m_dataType = MappingTypeConversion::deserializeDataType(j.at("dataType").get<std::string>());
        m_minimum = j.at("minimum").get<double>();
        m_maximum = j.at("maximum").get<double>();
        if (m_registerType == ModbusRegisterMapping::RegisterType::HOLDING_REGISTER_ACTUATOR)
        {
            if (m_mappingType == ModbusRegisterMapping::MappingType::CONFIGURATION)
            {
                // or LabelMap or address
                // if both, make an error, or if none, make an error
                bool gotAddress, gotLabelMap;

                try
                {
                    m_address = j.at("address").get<int>();
                    gotAddress = true;
                }
                catch (std::exception&)
                {
                    gotAddress = false;
                }

                try
                {
                    auto tempLabelMap = std::map<std::string, int>(j.at("labelMap").get<std::map<std::string, int>>());
                    typedef std::function<bool(std::pair<std::string, int>, std::pair<std::string, int>)> Comparator;
                    Comparator compFunctor = [](const std::pair<std::string, int>& elem1,
                                                const std::pair<std::string, int>& elem2) {
                        return elem1.second < elem2.second;
                    };
                    std::set<std::pair<std::string, int>, Comparator> setOfWords(tempLabelMap.begin(),
                                                                                 tempLabelMap.end(), compFunctor);

                    // labelMap was already initialized
                    for (std::pair<std::string, int> element : setOfWords)
                    {
                        m_labelsAndAddresses.emplace_back(element);
                    }
                    gotLabelMap = true;
                }
                catch (std::exception&)
                {
                    gotLabelMap = false;
                }

                if (gotAddress == gotLabelMap)
                {
                    if (gotAddress)
                    {
                        throw std::logic_error("You cannot set both address and a labelMap for " + m_name +
                                               " !"
                                               " Choose only address to make configuration single-value, or "
                                               "labelMap to make configuration multi-value!");
                    }
                    else
                    {
                        throw std::logic_error("You have to put either address or labelMap for " + m_name +
                                               " !"
                                               " Choose either address to make configuration single-value, or"
                                               "labelMap to make configuration multi-value!");
                    }
                }
            }
            else
            {
                // if it's not a configuration, we need the address no matter what
                m_address = j.at("address").get<int>();
            }
        }
        else
        {
            m_address = j.at("address").get<int>();
        }
    }
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
        int min = m_labelsAndAddresses.begin()->second;
        for (auto& label : m_labelsAndAddresses)
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

ModbusRegisterMapping::MappingType ModbusRegisterMapping::getMappingType() const
{
    return m_mappingType;
}

LabelMap ModbusRegisterMapping::getLabelsAndAddresses() const
{
    return m_labelsAndAddresses;
}

bool ModbusRegisterMapping::update(const std::string& newValue)
{
    bool isValueUpdated = m_value != newValue;
    m_value = newValue;

    bool isValueInitialized = m_isInitialized;
    m_isInitialized = true;

    bool isValid = m_isValid;
    m_isValid = true;

    return !isValueInitialized || isValueUpdated || !isValid;
}

const std::string& ModbusRegisterMapping::getValue() const
{
    return m_value;
}

void ModbusRegisterMapping::setValid(bool valid)
{
    m_isValid = valid;
}

bool ModbusRegisterMapping::update(signed short newRegisterValue)
{
    const auto newRegisterValueStr = std::to_string(newRegisterValue);
    return update(newRegisterValueStr);
}

bool ModbusRegisterMapping::update(unsigned short newRegisterValue)
{
    const auto newRegisterValueStr = std::to_string(newRegisterValue);
    return update(newRegisterValueStr);
}

bool ModbusRegisterMapping::update(float newRegisterValue)
{
    const auto newRegisterValueStr = std::to_string(newRegisterValue);
    return update(newRegisterValueStr);
}

bool ModbusRegisterMapping::update(bool newRegisterValue)
{
    const auto newRegisterValueStr = std::to_string(newRegisterValue);
    return update(newRegisterValueStr);
}

bool ModbusRegisterMapping::update(const std::vector<bool>& newRegisterValue)
{
    std::string newRegisterValueStr;
    for (int i = 0; i < newRegisterValue.size(); i++)
    {
        newRegisterValueStr += newRegisterValue[i] ? "true" : "false";
        if (i < newRegisterValue.size() - 1)
        {
            newRegisterValueStr += ',';
        }
    }
    return update(newRegisterValueStr);
}

bool ModbusRegisterMapping::update(const std::vector<short>& newRegisterValue)
{
    std::string newRegisterValueStr;
    for (int i = 0; i < newRegisterValue.size(); i++)
    {
        newRegisterValueStr += std::to_string(newRegisterValue[i]);
        if (i < newRegisterValue.size() - 1)
        {
            newRegisterValueStr += ',';
        }
    }
    return update(newRegisterValueStr);
}

bool ModbusRegisterMapping::update(const std::vector<unsigned short>& newRegisterValue)
{
    std::string newRegisterValueStr;
    for (int i = 0; i < newRegisterValue.size(); i++)
    {
        newRegisterValueStr += std::to_string(newRegisterValue[i]);
        if (i < newRegisterValue.size() - 1)
        {
            newRegisterValueStr += ',';
        }
    }
    return update(newRegisterValueStr);
}

bool ModbusRegisterMapping::update(const std::vector<float>& newRegisterValue)
{
    std::string newRegisterValueStr;
    for (int i = 0; i < newRegisterValue.size(); i++)
    {
        newRegisterValueStr += std::to_string(newRegisterValue[i]);
        if (i < newRegisterValue.size() - 1)
        {
            newRegisterValueStr += ',';
        }
    }
    return update(newRegisterValueStr);
}

int ModbusRegisterMapping::getSlaveAddress() const
{
    return static_cast<int>(m_slaveAddress);
}

void ModbusRegisterMapping::setSlaveAddress(uint slaveAddress)
{
    m_slaveAddress = slaveAddress;
}

ModbusRegisterMapping::RegisterType MappingTypeConversion::deserializeRegisterType(const std::string& registerType)
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

ModbusRegisterMapping::DataType MappingTypeConversion::deserializeDataType(const std::string& dataType)
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

ModbusRegisterMapping::MappingType MappingTypeConversion::deserializeMappingType(const std::string& mappingType)
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
