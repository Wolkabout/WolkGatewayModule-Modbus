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

#ifndef MODBUSREGISTERMAPPING_H
#define MODBUSREGISTERMAPPING_H

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
    enum class RegisterType
    {
        COIL = 0,
        INPUT_CONTACT,
        INPUT_REGISTER,
        HOLDING_REGISTER_SENSOR,
        HOLDING_REGISTER_ACTUATOR
    };

    enum class MappingType
    {
        DEFAULT,
        SENSOR,
        ACTUATOR,
        ALARM,
        CONFIGURATION
    };

    enum class DataType
    {
        INT16 = 0,
        UINT16,
        REAL32,
        BOOL
    };

    ModbusRegisterMapping(std::string name, std::string reference, std::string description, double minimum,
                          double maximum, int address, RegisterType registerType, DataType dataType,
                          MappingType mappingType);

    ModbusRegisterMapping(std::string name, std::string reference, std::string description, double minimum,
                          double maximum, const LabelMap& labelsAndAddresses, RegisterType registerType,
                          DataType dataType);

    ModbusRegisterMapping(nlohmann::json j);

    const std::string& getName() const;
    const std::string& getReference() const;
    const std::string& getDescription() const;

    double getMinimum() const;
    double getMaximum() const;

    LabelMap getLabelsAndAddresses() const;
    int getAddress() const;
    int getSlaveAddress() const { return -1; }

    RegisterType getRegisterType() const;
    DataType getDataType() const;
    MappingType getMappingType() const;

private:
    std::string m_name;
    std::string m_reference;
    std::string m_description;

    double m_minimum = 0.0;
    double m_maximum = 1.0;

    int m_address = -1;
    LabelMap m_labelsAndAddresses{};

    RegisterType m_registerType;
    DataType m_dataType;
    MappingType m_mappingType;
};

class MappingTypeConversion
{
public:
    static ModbusRegisterMapping::RegisterType deserializeRegisterType(const std::string& registerType);
    static ModbusRegisterMapping::DataType deserializeDataType(const std::string& dataType);
    static ModbusRegisterMapping::MappingType deserializeMappingType(const std::string& mappingType);
};
}    // namespace wolkabout

#endif
