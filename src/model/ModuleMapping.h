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

#include "core/utilities/json.hpp"
#include "more_modbus/RegisterMapping.h"

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
using nlohmann::json;
typedef std::vector<std::pair<std::string, int>> LabelMap;

/**
 * @brief Model class representing information necessary to create a mapping.
 */
class ModuleMapping
{
public:
    /**
     * @brief Enumeration that provides the ability to override the mapping type that will
     *        be created for the platform.
     */
    enum class MappingType
    {
        DEFAULT,
        SENSOR,
        ACTUATOR,
        ALARM,
        CONFIGURATION
    };

    ModuleMapping(const ModuleMapping& mapping) = default;

    explicit ModuleMapping(nlohmann::json j);

    bool isReadRestricted() const;

    const std::string& getName() const;
    const std::string& getReference() const;
    const std::string& getDescription() const;

    double getMinimum() const;
    double getMaximum() const;
    double getDeadbandValue() const;
    std::chrono::milliseconds getFrequencyFilterValue() const;

    int getAddress() const;
    LabelMap getLabelMap() const;
    int getBitIndex() const;
    int getRegisterCount() const;

    RegisterMapping::RegisterType getRegisterType() const;
    RegisterMapping::OutputType getDataType() const;
    RegisterMapping::OperationType getOperationType() const;
    MappingType getMappingType() const;

private:
    bool m_readRestricted;
    std::string m_name;
    std::string m_reference;
    std::string m_description;

    double m_minimum = 0.0;
    double m_maximum = 1.0;
    double m_deadbandValue = 0.0;
    std::chrono::milliseconds m_frequencyFilterValue = std::chrono::milliseconds(0);

    int m_address = -1;
    LabelMap m_labelMap{};
    int m_bitIndex = -1;
    int m_addressCount = -1;

    RegisterMapping::RegisterType m_registerType;
    RegisterMapping::OutputType m_dataType;
    RegisterMapping::OperationType m_operationType;
    MappingType m_mappingType;
};

/**
 * @brief Utility methods for parsing enumerations in ModuleMapping.
 */
class MappingTypeConversion
{
public:
    /**
     * @brief Parse the mapping type parameter from string into an enumeration value.
     * @param mappingType represented as a string file.
     * @return mappingType parsed to enumeration.
     */
    static ModuleMapping::MappingType deserializeMappingType(const std::string& mappingType);
};
}    // namespace wolkabout

#endif
