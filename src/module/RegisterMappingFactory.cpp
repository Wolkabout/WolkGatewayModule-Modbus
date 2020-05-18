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

#include "RegisterMappingFactory.h"
#include "mappings/BoolMapping.h"
#include "mappings/FloatMapping.h"
#include "mappings/Int16Mapping.h"
#include "mappings/Int32Mapping.h"
#include "mappings/StringMapping.h"
#include "mappings/UInt16Mapping.h"
#include "mappings/UInt32Mapping.h"

namespace wolkabout
{
std::shared_ptr<RegisterMapping> RegisterMappingFactory::fromJSONMapping(const ModuleMapping& jsonMapping)
{
    switch (jsonMapping.getDataType())
    {
    case RegisterMapping::OutputType::BOOL:
        if (jsonMapping.getOperationType() == RegisterMapping::OperationType::TAKE_BIT)
        {
            return std::make_shared<BoolMapping>(jsonMapping.getReference(), jsonMapping.getRegisterType(),
                                                 jsonMapping.getAddress(), jsonMapping.getOperationType(),
                                                 jsonMapping.getBitIndex(), jsonMapping.isReadRestricted());
        }
        return std::make_shared<BoolMapping>(jsonMapping.getReference(), jsonMapping.getRegisterType(),
                                             jsonMapping.getAddress(), jsonMapping.isReadRestricted());
    case RegisterMapping::OutputType::UINT16:
        return std::make_shared<UInt16Mapping>(jsonMapping.getReference(), jsonMapping.getRegisterType(),
                                               jsonMapping.getAddress(), jsonMapping.isReadRestricted());
    case RegisterMapping::OutputType::INT16:
        return std::make_shared<Int16Mapping>(jsonMapping.getReference(), jsonMapping.getRegisterType(),
                                              jsonMapping.getAddress(), jsonMapping.isReadRestricted());
    case RegisterMapping::OutputType::UINT32:
        return std::make_shared<UInt32Mapping>(jsonMapping.getReference(), jsonMapping.getRegisterType(),
                                               std::vector<int16_t>{static_cast<short>(jsonMapping.getAddress()),
                                                                    static_cast<short>(jsonMapping.getAddress() + 1)},
                                               jsonMapping.getOperationType(), jsonMapping.isReadRestricted());
    case RegisterMapping::OutputType::INT32:
        return std::make_shared<Int32Mapping>(jsonMapping.getReference(), jsonMapping.getRegisterType(),
                                              std::vector<int16_t>{static_cast<short>(jsonMapping.getAddress()),
                                                                   static_cast<short>(jsonMapping.getAddress() + 1)},
                                              jsonMapping.getOperationType(), jsonMapping.isReadRestricted());
    case RegisterMapping::OutputType::FLOAT:
        return std::make_shared<FloatMapping>(jsonMapping.getReference(), jsonMapping.getRegisterType(),
                                              std::vector<int16_t>{static_cast<short>(jsonMapping.getAddress()),
                                                                   static_cast<short>(jsonMapping.getAddress() + 1)},
                                              jsonMapping.isReadRestricted());
    case RegisterMapping::OutputType::STRING:
        std::vector<int16_t> addresses;
        const auto startAddress = static_cast<int16_t>(jsonMapping.getAddress());
        for (int i = 0; i < jsonMapping.getRegisterCount(); i++)
        {
            addresses.emplace_back(startAddress + i);
        }
        return std::make_shared<StringMapping>(jsonMapping.getReference(), jsonMapping.getRegisterType(), addresses,
                                               jsonMapping.getOperationType(), jsonMapping.isReadRestricted());
    }
    return nullptr;
}
}    // namespace wolkabout
