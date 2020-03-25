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

#ifndef MODBUSREGISTERMAPPING_H
#define MODBUSREGISTERMAPPING_H

#include "RegisterMapping.h"
#include "utilities/json.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
using nlohmann::json;
typedef std::vector<std::pair<std::string, int>> LabelMap;

class ModbusRegisterMapping
{
public:
    enum class MappingType
    {
        DEFAULT,
        SENSOR,
        ACTUATOR,
        ALARM,
        CONFIGURATION
    };

    ModbusRegisterMapping(std::string name, std::string reference, std::string description, double minimum,
                          double maximum, int address, RegisterMapping::RegisterType registerType,
                          RegisterMapping::OutputType dataType, MappingType mappingType);

    ModbusRegisterMapping(std::string name, std::string reference, std::string description, double minimum,
                          double maximum, const LabelMap& labelsAndAddresses,
                          RegisterMapping::RegisterType registerType, RegisterMapping::OutputType dataType);

    ModbusRegisterMapping(const std::string& name, const std::string& reference, const std::string& description,
                          double minimum, double maximum, const std::vector<int>& addresses,
                          RegisterMapping::RegisterType registerType, RegisterMapping::OutputType dataType,
                          MappingType mappingType);

    ModbusRegisterMapping(const ModbusRegisterMapping& mapping) = default;

    explicit ModbusRegisterMapping(nlohmann::json j);

    const std::string& getName() const;
    const std::string& getReference() const;
    const std::string& getDescription() const;

    double getMinimum() const;
    double getMaximum() const;

    LabelMap getLabelsAndAddresses() const;
    int getAddress() const;
    int getRegisterCount() const;

    RegisterMapping::RegisterType getRegisterType() const;
    RegisterMapping::OutputType getDataType() const;
    MappingType getMappingType() const;

private:
    std::string m_name;
    std::string m_reference;
    std::string m_description;

    double m_minimum = 0.0;
    double m_maximum = 1.0;

    int m_address = -1;
    LabelMap m_labelsAndAddresses{};
    std::vector<int> m_addresses;

    RegisterMapping::RegisterType m_registerType;
    RegisterMapping::OutputType m_dataType;
    MappingType m_mappingType;
};

class MappingTypeConversion
{
public:
    static ModbusRegisterMapping::MappingType deserializeMappingType(const std::string& mappingType);
};
}    // namespace wolkabout

#endif
