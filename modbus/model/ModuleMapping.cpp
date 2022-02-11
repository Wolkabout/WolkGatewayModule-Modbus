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

#include "modbus/model/ModuleMapping.h"

#include "core/utilities/Logger.h"
#include "modbus/utilities/JsonReaderParser.h"
#include "more_modbus/utilities/Deserializers.h"

#include <stdexcept>
#include <string>

namespace wolkabout
{
namespace modbus
{
using nlohmann::json;

ModuleMapping::ModuleMapping(nlohmann::json j)
: m_name{JsonReaderParser::read<std::string>(j, "name")}
, m_reference{JsonReaderParser::read<std::string>(j, "reference")}
, m_registerType{more_modbus::registerTypeFromString(JsonReaderParser::read<std::string>(j, "registerType"))}
, m_dataType{more_modbus::outputTypeFromString(JsonReaderParser::read<std::string>(j, "dataType"))}
, m_operationType{more_modbus::operationTypeFromString(
    JsonReaderParser::readOrDefault(j, "operationType", std::string{}))}
, m_mappingType{mappingTypeFromString(JsonReaderParser::readOrDefault(j, "mappingType", std::string{}))}
, m_address{JsonReaderParser::read<std::uint16_t>(j, "address")}
, m_bitIndex{JsonReaderParser::readOrDefault<std::uint16_t>(j, "bitIndex", static_cast<std::uint16_t>(-1))}
, m_addressCount{JsonReaderParser::readOrDefault<std::uint16_t>(j, "addressCount", static_cast<std::uint16_t>(-1))}
, m_deadbandValue{JsonReaderParser::readOrDefault(j, "deadbandValue", 0.0)}
, m_frequencyFilterValue{JsonReaderParser::readOrDefault(j, "frequencyFilterValue", 0)}
, m_repeat{JsonReaderParser::readOrDefault(j, "repeat", 0)}
, m_defaultValue{JsonReaderParser::readTypedValue(j, "defaultValue")}
, m_safeMode{j.find("safeMode") != j.end()}
, m_safeModeValue{JsonReaderParser::readTypedValue(j, "safeMode")}
{
    // Now attempt to read the repeat and default value
    if (m_repeat.count() > 0 && j.find("defaultValue") == j.end())
        throw std::runtime_error("You can not create a `repeat` mapping without a `defaultValue`.");

    // Check that the safe mode is not enabled on a bool mapping
    if (m_safeMode && (m_registerType == more_modbus::RegisterType::INPUT_REGISTER ||
                       m_registerType == more_modbus::RegisterType::INPUT_CONTACT))
        throw std::runtime_error("You can not create a `safeMode` mapping with a read-only register.");
}

const std::string& ModuleMapping::getName() const
{
    return m_name;
}

const std::string& ModuleMapping::getReference() const
{
    return m_reference;
}

std::chrono::milliseconds ModuleMapping::getRepeat() const
{
    return m_repeat;
}

const std::string& ModuleMapping::getDefaultValue() const
{
    return m_defaultValue;
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
    return m_address;
}

int ModuleMapping::getBitIndex() const
{
    return m_bitIndex;
}

int ModuleMapping::getRegisterCount() const
{
    if (m_addressCount > 1)
        return m_addressCount;
    return 1;
}

more_modbus::RegisterType ModuleMapping::getRegisterType() const
{
    return m_registerType;
}

more_modbus::OutputType ModuleMapping::getDataType() const
{
    return m_dataType;
}

MappingType ModuleMapping::getMappingType() const
{
    return m_mappingType;
}

more_modbus::OperationType ModuleMapping::getOperationType() const
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
}    // namespace modbus
}    // namespace wolkabout
