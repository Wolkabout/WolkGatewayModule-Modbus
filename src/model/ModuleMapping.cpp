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
#include "utilities/JsonReaderParser.h"
#include "utilities/Logger.h"

#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace wolkabout
{
using nlohmann::json;

ModuleMapping::ModuleMapping(nlohmann::json j)
{
    m_readRestricted = JsonReaderParser::readOrDefault(j, "writeOnly", false);
    m_name = j.at("name").get<std::string>();
    m_reference = j.at("reference").get<std::string>();
    m_description = JsonReaderParser::readOrDefault(j, "description", "");

    m_minimum = JsonReaderParser::readOrDefault(j, "minimum", 0.0);
    m_maximum = JsonReaderParser::readOrDefault(j, "maximum", 1.0);

    m_address = JsonReaderParser::readOrDefault(j, "address", -1);
    try
    {
        auto tempLabelMap = std::map<std::string, int>(j.at("labelMap").get<std::map<std::string, int>>());
        typedef std::function<bool(std::pair<std::string, int>, std::pair<std::string, int>)> Comparator;
        Comparator compFunctor = [](const std::pair<std::string, int>& elem1,
                                    const std::pair<std::string, int>& elem2) { return elem1.second < elem2.second; };
        std::set<std::pair<std::string, int>, Comparator> setOfWords(tempLabelMap.begin(), tempLabelMap.end(),
                                                                     compFunctor);

        // labelMap was already initialized
        for (const auto& element : setOfWords)
        {
            m_labelMap.emplace_back(element);
        }
    }
    catch (std::exception&)
    {
        m_labelMap.clear();
    }
    m_bitIndex = JsonReaderParser::readOrDefault(j, "bitIndex", -1);
    m_addressCount = JsonReaderParser::readOrDefault(j, "addressCount", -1);

    m_registerType = Deserializers::deserializeRegisterType(j.at("registerType").get<std::string>());
    m_dataType = Deserializers::deserializeDataType(j.at("dataType").get<std::string>());

    std::string m_operationTypeString = JsonReaderParser::readOrDefault(j, "operationType", "");
    if (m_operationTypeString.empty())
        m_operationType = RegisterMapping::OperationType::NONE;
    else
        m_operationType = Deserializers::deserializeOperationType(m_operationTypeString);

    std::string m_mappingTypeString = JsonReaderParser::readOrDefault(j, "mappingType", "");
    if (m_mappingTypeString.empty())
        m_mappingType = ModuleMapping::MappingType::DEFAULT;
    else
        m_mappingType = MappingTypeConversion::deserializeMappingType(m_mappingTypeString);
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
    if (m_address != -1)
        return m_address;

    int min = m_labelMap.begin()->second;
    for (auto& label : m_labelMap)
    {
        if (label.second < min)
        {
            min = label.second;
        }
    }
    return min;
}

int ModuleMapping::getBitIndex() const
{
    return m_bitIndex;
}

int ModuleMapping::getRegisterCount() const
{
    if (m_addressCount != -1)
        return m_addressCount;
    else if (m_address != -1)
        return 1;
    return static_cast<int>(m_labelMap.size());
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

LabelMap ModuleMapping::getLabelMap() const
{
    return m_labelMap;
}

bool ModuleMapping::isReadRestricted() const
{
    return m_readRestricted;
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
