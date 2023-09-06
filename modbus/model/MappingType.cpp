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

#include "modbus/model/MappingType.h"

#include <algorithm>

namespace wolkabout::modbus
{
MappingType mappingTypeFromString(std::string value)
{
    std::transform(value.cbegin(), value.cend(), value.begin(), ::toupper);
    if (value == "READONLY")
        return MappingType::ReadOnly;
    else if (value == "READWRITE")
        return MappingType::ReadWrite;
    else if (value == "WRITEONLY")
        return MappingType::WriteOnly;
    else if (value == "ATTRIBUTE")
        return MappingType::Attribute;
    return MappingType::Default;
}
}    // namespace wolkabout::modbus
