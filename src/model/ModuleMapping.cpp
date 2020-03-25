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

#include "ModuleMapping.h"
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

ModuleMapping::ModuleMapping(std::string name, std::string reference, std::string description, double minimum,
                             double maximum, int address, RegisterMapping::RegisterType registerType,
                             RegisterMapping::OutputType dataType,
                             wolkabout::ModuleMapping::MappingType mappingType = MappingType::DEFAULT,
                             bool readRestricted)
: m_readRestricted(readRestricted)
, m_name(std::move(name))
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
    m_mappings.emplace_back(std::make_shared<RegisterMapping>(reference, registerType, address,
                                                              RegisterMapping::OperationType::NONE, readRestricted));
}

ModuleMapping::ModuleMapping(std::string name, std::string reference, std::string description, double minimum,
                             double maximum, int address, int bitIndex, RegisterMapping::RegisterType registerType,
                             ModuleMapping::MappingType mappingType, bool readRestricted)
: m_readRestricted(readRestricted)
, m_name(std::move(name))
, m_reference(std::move(reference))
, m_description(std::move(description))
, m_minimum(minimum)
, m_maximum(maximum)
, m_address(address)
, m_labelsAndAddresses()
, m_addresses()
, m_bitIndex(bitIndex)
, m_registerType(registerType)
, m_dataType(RegisterMapping::OutputType::BOOL)
, m_mappingType(mappingType)
{
    m_mappings.emplace_back(std::make_shared<RegisterMapping>(
      reference, registerType, address, RegisterMapping::OperationType::TAKE_BIT, bitIndex, readRestricted));
}

ModuleMapping::ModuleMapping(std::string name, std::string reference, std::string description, double minimum,
                             double maximum, const LabelMap& labelsAndAddresses,
                             RegisterMapping::RegisterType registerType, RegisterMapping::OutputType dataType,
                             bool readRestricted)
: m_readRestricted(readRestricted)
, m_name(std::move(name))
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
    for (const auto& pair : labelsAndAddresses)
    {
        m_mappings.emplace_back(std::make_shared<RegisterMapping>(
          pair.first, registerType, pair.second, RegisterMapping::OperationType::NONE, readRestricted));
    }
}

ModuleMapping::ModuleMapping(const std::string& name, const std::string& reference, const std::string& description,
                             double minimum, double maximum, const std::vector<int16_t>& addresses,
                             RegisterMapping::RegisterType registerType, RegisterMapping::OutputType dataType,
                             RegisterMapping::OperationType operationType, ModuleMapping::MappingType mappingType,
                             bool readRestricted)
: m_readRestricted(readRestricted)
, m_name(name)
, m_reference(reference)
, m_description(description)
, m_minimum(minimum)
, m_maximum(maximum)
, m_addresses(addresses)
, m_registerType(registerType)
, m_dataType(dataType)
, m_mappingType(mappingType)
{
    m_mappings.emplace_back(
      std::make_shared<RegisterMapping>(reference, registerType, addresses, dataType, operationType, readRestricted));
}

ModuleMapping::ModuleMapping(nlohmann::json j)
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
        m_mappingType = ModuleMapping::MappingType::DEFAULT;
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
            if (m_mappingType == ModuleMapping::MappingType::CONFIGURATION)
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

const std::string& ModuleMapping::getName() const
{
    return m_name;
}

const std::string& ModuleMapping::getReference() const
{
    return m_reference;
}

const std::string& ModuleMapping::getDescription() const
{
    return m_description;
}

double ModuleMapping::getMinimum() const
{
    return m_minimum;
}

double ModuleMapping::getMaximum() const
{
    return m_maximum;
}

int ModuleMapping::getAddress() const
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

int ModuleMapping::getRegisterCount() const
{
    if (m_address == -1)
    {
        return static_cast<int>(m_labelsAndAddresses.size());
    }
    return 1;
}

RegisterMapping::RegisterType ModuleMapping::getRegisterType() const
{
    return m_registerType;
}

RegisterMapping::OutputType ModuleMapping::getDataType() const
{
    return m_dataType;
}

ModuleMapping::MappingType ModuleMapping::getMappingType() const
{
    return m_mappingType;
}

LabelMap ModuleMapping::getLabelsAndAddresses() const
{
    return m_labelsAndAddresses;
}

const std::vector<std::shared_ptr<RegisterMapping>>& ModuleMapping::getMappings() const
{
    return m_mappings;
}

bool ModuleMapping::isReadRestricted() const
{
    return m_readRestricted;
}

const std::vector<uint16_t>& ModuleMapping::getAddresses() const
{
    return m_addresses;
}

RegisterMapping::OperationType ModuleMapping::getOperationType() const
{
    return m_operationType;
}

ModuleMapping::MappingType MappingTypeConversion::deserializeMappingType(const std::string& mappingType)
{
    if (mappingType == "DEFAULT")
    {
        return ModuleMapping::MappingType::DEFAULT;
    }
    else if (mappingType == "SENSOR")
    {
        return ModuleMapping::MappingType::SENSOR;
    }
    else if (mappingType == "ACTUATOR")
    {
        return ModuleMapping::MappingType::ACTUATOR;
    }
    else if (mappingType == "ALARM")
    {
        return ModuleMapping::MappingType::ALARM;
    }
    else if (mappingType == "CONFIGURATION")
    {
        return ModuleMapping::MappingType::CONFIGURATION;
    }
    throw std::logic_error("Unknown data type: " + mappingType);
}
}    // namespace wolkabout
