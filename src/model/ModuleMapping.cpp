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

#include "core/utilities/Logger.h"
#include "more_modbus/utilities/Deserializers.h"
#include "utilities/JsonReaderParser.h"

#include <stdexcept>
#include <string>

namespace wolkabout
{
using nlohmann::json;

ModuleMapping::ModuleMapping(nlohmann::json j)
{
    m_readRestricted = JsonReaderParser::readOrDefault(j, "writeOnly", false);
    m_name = j.at("name").get<std::string>();
    m_reference = j.at("reference").get<std::string>();
    m_description = JsonReaderParser::readOrDefault(j, "description", "");

    if (j.find("minimum") != j.end() || j.find("maximum") != j.end())
        LOG(WARN) << "Minimum and maximum are disabled by the Wolkabout protocol, they will not be used.";

    m_deadbandValue = JsonReaderParser::readOrDefault(j, "deadbandValue", 0.0);
    m_frequencyFilterValue =
      static_cast<std::chrono::milliseconds>(JsonReaderParser::readOrDefault(j, "frequencyFilterValue", 0));

    m_address = JsonReaderParser::readOrDefault(j, "address", -1);

    if (j.find("labelMap") != j.end())
        throw std::runtime_error("The multi-value configurations are not enabled by the Wolkabout protocol!");

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

    // Now attempt to read the repeat and default value
    m_repeat = static_cast<uint64_t>(JsonReaderParser::readOrDefault(j, "repeat", 0));

    if (j.find("defaultValue") != j.end())
    {
        switch (j.at("defaultValue").type())
        {
        case json::value_t::number_integer:
        case json::value_t::number_unsigned:
            m_defaultValue = std::to_string(JsonReaderParser::readOrDefault(j, "defaultValue", 0));
            break;
        case json::value_t::number_float:
            m_defaultValue = std::to_string(JsonReaderParser::readOrDefault(j, "defaultValue", double(0.0)));
            break;
        case json::value_t::boolean:
            m_defaultValue = j.at("defaultValue").get<bool>() ? "true" : "false";
            break;
        default:
            m_defaultValue = JsonReaderParser::readOrDefault(j, "defaultValue", "");
            break;
        }
    }

    // And safe mode
    m_safeMode = false;
    if (j.find("safeMode") != j.end())
    {
        if (m_registerType == RegisterMapping::RegisterType::INPUT_REGISTER ||
            m_registerType == RegisterMapping::RegisterType::INPUT_CONTACT)
            throw std::runtime_error("You can not create a read-only register with a safe mode value -> '" +
                                     m_reference + "'.");

        m_safeMode = true;

        switch (j.at("safeMode").type())
        {
        case json::value_t::number_integer:
        case json::value_t::number_unsigned:
            m_safeModeValue = std::to_string(JsonReaderParser::readOrDefault(j, "safeMode", 0));
            break;
        case json::value_t::number_float:
            m_safeModeValue = std::to_string(JsonReaderParser::readOrDefault(j, "safeMode", double(0.0)));
            break;
        case json::value_t::boolean:
            m_safeModeValue = j.at("safeMode").get<bool>() ? "true" : "false";
            break;
        default:
            m_safeModeValue = JsonReaderParser::readOrDefault(j, "safeMode", "");
            break;
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

uint64_t ModuleMapping::getRepeat() const
{
    return m_repeat;
}

const std::string& ModuleMapping::getDefaultValue() const
{
    return m_defaultValue;
}

double ModuleMapping::getMinimum() const
{
    return m_minimum;
}

double ModuleMapping::getMaximum() const
{
    return m_maximum;
}

double ModuleMapping::getDeadbandValue() const
{
    return m_deadbandValue;
}

std::chrono::milliseconds ModuleMapping::getFrequencyFilterValue() const
{
    return m_frequencyFilterValue;
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

bool ModuleMapping::hasSafeMode() const
{
    return m_safeMode;
}

const std::string& ModuleMapping::getSafeModeValue() const
{
    return m_safeModeValue;
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
