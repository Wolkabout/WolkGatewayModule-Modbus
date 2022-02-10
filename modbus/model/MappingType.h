/**
 * Copyright 2022 Wolkabout Technology s.r.o.
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

#ifndef WOLKGATEWAYMODBUSMODULE_MAPPINGTYPE_H
#define WOLKGATEWAYMODBUSMODULE_MAPPINGTYPE_H

#include <string>

namespace wolkabout
{
namespace modbus
{
// This is the enumeration that describes the type of the mapping based on data access to the platform.
enum class MappingType
{
    Default = -1,
    ReadOnly,
    ReadWrite,
    WriteOnly,
    Attribute
};

/**
 * Helper method used to convert a string value into the enumeration value for a MappingType.
 *
 * @param value The string value.
 * @return The parsed MappingType value.
 */
MappingType mappingTypeFromString(std::string value);
}    // namespace modbus
}    // namespace wolkabout

#endif    // WOLKGATEWAYMODBUSMODULE_MAPPINGTYPE_H
