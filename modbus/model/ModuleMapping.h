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
#include "modbus/model/MappingType.h"
#include "more_modbus/RegisterMapping.h"

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
namespace modbus
{
/**
 * @brief Model class representing information necessary to create a mapping.
 */
class ModuleMapping
{
public:
    ModuleMapping(const ModuleMapping& mapping) = default;

    explicit ModuleMapping(nlohmann::json j);

    bool isReadRestricted() const;

    const std::string& getName() const;
    const std::string& getReference() const;

    std::chrono::milliseconds getRepeat() const;
    const std::string& getDefaultValue() const;

    bool hasSafeMode() const;
    const std::string& getSafeModeValue() const;

    double getDeadbandValue() const;
    std::chrono::milliseconds getFrequencyFilterValue() const;

    int getAddress() const;
    int getBitIndex() const;
    int getRegisterCount() const;

    more_modbus::RegisterType getRegisterType() const;
    more_modbus::OutputType getDataType() const;
    more_modbus::OperationType getOperationType() const;
    MappingType getMappingType() const;

private:
    // Identifying information
    std::string m_name;
    std::string m_reference;

    // Register information
    more_modbus::RegisterType m_registerType;
    more_modbus::OutputType m_dataType;
    more_modbus::OperationType m_operationType;
    MappingType m_mappingType;

    std::uint16_t m_address;
    std::uint16_t m_bitIndex;
    std::uint16_t m_addressCount;

    // Deadband filtering information
    double m_deadbandValue;
    std::chrono::milliseconds m_frequencyFilterValue;

    // Repeat write information
    std::chrono::milliseconds m_repeat;
    std::string m_defaultValue;

    // Safe mode information
    bool m_safeMode;
    std::string m_safeModeValue;
};
}    // namespace modbus
}    // namespace wolkabout

#endif
