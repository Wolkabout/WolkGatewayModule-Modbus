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

#include "ModbusRegisterMapping.h"
#include "utilities/Deserializers.h"
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
  RegisterMapping::RegisterType registerType, RegisterMapping::OutputType dataType,
  wolkabout::ModbusRegisterMapping::MappingType mappingType = MappingType::DEFAULT)
: m_name(std::move(name))
, m_reference(std::move(reference))
, m_description(std::move(description))
, m_minimum(minimum)
, m_maximum(maximum)
, m_address(address)
, m_labelsAndAddresses()
, m_addresses()
, m_registerType(registerType)
, m_dataType(dataType)
, m_mappingType(mappingType)
{
}

ModbusRegisterMapping::ModbusRegisterMapping(std::string name, std::string reference, std::string description,
                                             double minimum, double maximum, const LabelMap& labelsAndAddresses,
                                             RegisterMapping::RegisterType registerType,
                                             RegisterMapping::OutputType dataType)
: m_name(std::move(name))
, m_reference(std::move(reference))
, m_description(std::move(description))
, m_minimum(minimum)
, m_maximum(maximum)
, m_address(-1)
, m_labelsAndAddresses(labelsAndAddresses)
, m_addresses()
, m_registerType(registerType)
, m_dataType(dataType)
, m_mappingType(MappingType::CONFIGURATION)
{
}

ModbusRegisterMapping::ModbusRegisterMapping(const std::string& name, const std::string& reference,
                                             const std::string& description, double minimum, double maximum,
                                             const std::vector<int>& addresses,
                                             RegisterMapping::RegisterType registerType,
                                             RegisterMapping::OutputType dataType,
                                             ModbusRegisterMapping::MappingType mappingType)
: m_name(name)
, m_reference(reference)
, m_description(description)
, m_minimum(minimum)
, m_maximum(maximum)
, m_addresses(addresses)
, m_registerType(registerType)
, m_dataType(dataType)
, m_mappingType(mappingType)
{
}

ModbusRegisterMapping::ModbusRegisterMapping(nlohmann::json j)
{
    m_name = j.at("name").get<std::string>();
    m_reference = j.at("reference").get<std::string>();
    m_registerType = Deserializers::deserializeRegisterType(j.at("registerType").get<std::string>());

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

    if (m_registerType == RegisterMapping::RegisterType::COIL ||
        m_registerType == RegisterMapping::RegisterType::INPUT_CONTACT)
    {
        // this is obligatory here, we don't support multi-value boolean configurations
        m_address = j.at("address").get<int>();
        m_dataType = RegisterMapping::OutputType::BOOL;
        // we ignore dataType and min/max
    }
    else
    {
        m_dataType = Deserializers::deserializeDataType(j.at("dataType").get<std::string>());
        m_minimum = j.at("minimum").get<double>();
        m_maximum = j.at("maximum").get<double>();
        if (m_registerType == RegisterMapping::RegisterType::HOLDING_REGISTER)
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
                    for (const auto& element : setOfWords)
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
                                               "!"
                                               " Choose only address to make configuration single-value, or "
                                               "labelMap to make configuration multi-value!");
                    }
                    else
                    {
                        throw std::logic_error("You have to put either address or labelMap for " + m_name +
                                               "!"
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

int ModbusRegisterMapping::getRegisterCount() const
{
    if (m_address == -1)
    {
        return static_cast<int>(m_labelsAndAddresses.size());
    }
    return 1;
}

RegisterMapping::RegisterType ModbusRegisterMapping::getRegisterType() const
{
    return m_registerType;
}

RegisterMapping::OutputType ModbusRegisterMapping::getDataType() const
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
