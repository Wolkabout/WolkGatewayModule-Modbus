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

#include "more_modbus/mappings/BoolMapping.h"
#include "more_modbus/mappings/FloatMapping.h"
#include "more_modbus/mappings/Int16Mapping.h"
#include "more_modbus/mappings/Int32Mapping.h"
#include "more_modbus/mappings/StringMapping.h"
#include "more_modbus/mappings/UInt16Mapping.h"
#include "more_modbus/mappings/UInt32Mapping.h"

namespace wolkabout
{
std::shared_ptr<RegisterMapping> RegisterMappingFactory::fromJSONMapping(const ModuleMapping& jsonMapping)
{
    switch (jsonMapping.getDataType())
    {
    case RegisterMapping::OutputType::BOOL:
    {
        // Make place for the mapping that should be returned
        auto mapping = std::shared_ptr<BoolMapping>{};

        // Check if the default value is anything
        auto lowerCase = jsonMapping.getDefaultValue();
        std::transform(lowerCase.cbegin(), lowerCase.cend(), lowerCase.begin(), ::tolower);
        bool defaultValue;
        bool defaultSet = false;
        if (lowerCase == "true")
        {
            defaultValue = true;
            defaultSet = true;
        }
        else if (lowerCase == "false")
        {
            defaultValue = false;
            defaultSet = true;
        }

        // Make the mapping based on the operation
        if (defaultSet)
        {
            if (jsonMapping.getOperationType() == RegisterMapping::OperationType::TAKE_BIT)
            {
                mapping = std::make_shared<BoolMapping>(
                  jsonMapping.getReference(), jsonMapping.getRegisterType(), jsonMapping.getAddress(),
                  jsonMapping.getOperationType(), jsonMapping.getBitIndex(), jsonMapping.isReadRestricted(), -1,
                  jsonMapping.getFrequencyFilterValue(), jsonMapping.getRepeat(), &defaultValue);
            }
            else
            {
                mapping = std::make_shared<BoolMapping>(jsonMapping.getReference(), jsonMapping.getRegisterType(),
                                                        jsonMapping.getAddress(), jsonMapping.isReadRestricted(), -1,
                                                        jsonMapping.getFrequencyFilterValue(), jsonMapping.getRepeat(),
                                                        &defaultValue);
            }
        }
        else
        {
            if (jsonMapping.getOperationType() == RegisterMapping::OperationType::TAKE_BIT)
            {
                mapping = std::make_shared<BoolMapping>(jsonMapping.getReference(), jsonMapping.getRegisterType(),
                                                        jsonMapping.getAddress(), jsonMapping.getOperationType(),
                                                        jsonMapping.getBitIndex(), jsonMapping.isReadRestricted(), -1,
                                                        jsonMapping.getFrequencyFilterValue(), jsonMapping.getRepeat());
            }
            else
            {
                mapping = std::make_shared<BoolMapping>(jsonMapping.getReference(), jsonMapping.getRegisterType(),
                                                        jsonMapping.getAddress(), jsonMapping.isReadRestricted(), -1,
                                                        jsonMapping.getFrequencyFilterValue(), jsonMapping.getRepeat());
            }
        }
        return mapping;
    }
    case RegisterMapping::OutputType::UINT16:
    {
        // Make place for the mapping
        auto mapping = std::shared_ptr<UInt16Mapping>{};

        // Check if the default value is anything
        const auto& stringValue = jsonMapping.getDefaultValue();
        bool defaultSet = false;
        auto defaultValue = std::uint16_t{};
        try
        {
            defaultValue = static_cast<std::uint16_t>(std::stoul(stringValue));
            defaultSet = true;
        }
        catch (...)
        {
        }

        // Create the mapping
        if (defaultSet)
        {
            mapping = std::make_shared<UInt16Mapping>(
              jsonMapping.getReference(), jsonMapping.getRegisterType(), jsonMapping.getAddress(),
              jsonMapping.isReadRestricted(), -1, jsonMapping.getDeadbandValue(), jsonMapping.getFrequencyFilterValue(),
              jsonMapping.getRepeat(), &defaultValue);
        }
        else
        {
            // Make the mapping
            mapping = std::make_shared<UInt16Mapping>(jsonMapping.getReference(), jsonMapping.getRegisterType(),
                                                      jsonMapping.getAddress(), jsonMapping.isReadRestricted(), -1,
                                                      jsonMapping.getDeadbandValue(),
                                                      jsonMapping.getFrequencyFilterValue(), jsonMapping.getRepeat());
        }
        return mapping;
    }
    case RegisterMapping::OutputType::INT16:
    {
        // Make place for the mapping
        auto mapping = std::shared_ptr<Int16Mapping>{};

        // Check if the default value is anything
        const auto& stringValue = jsonMapping.getDefaultValue();
        bool defaultSet = false;
        auto defaultValue = std::int16_t{};
        try
        {
            defaultValue = static_cast<std::int16_t>(std::stoi(stringValue));
            defaultSet = true;
        }
        catch (...)
        {
        }

        // Create the mapping
        if (defaultSet)
        {
            mapping = std::make_shared<Int16Mapping>(
              jsonMapping.getReference(), jsonMapping.getRegisterType(), jsonMapping.getAddress(),
              jsonMapping.isReadRestricted(), -1, jsonMapping.getDeadbandValue(), jsonMapping.getFrequencyFilterValue(),
              jsonMapping.getRepeat(), &defaultValue);
        }
        else
        {
            // Make the mapping
            mapping = std::make_shared<Int16Mapping>(jsonMapping.getReference(), jsonMapping.getRegisterType(),
                                                     jsonMapping.getAddress(), jsonMapping.isReadRestricted(), -1,
                                                     jsonMapping.getDeadbandValue(),
                                                     jsonMapping.getFrequencyFilterValue(), jsonMapping.getRepeat());
        }
        return mapping;
    }
    case RegisterMapping::OutputType::UINT32:
    {
        // Make place for the mapping
        auto mapping = std::shared_ptr<UInt32Mapping>{};

        // Check if the default value is anything
        const auto& stringValue = jsonMapping.getDefaultValue();
        bool defaultSet = false;
        auto defaultValue = std::uint32_t{};
        try
        {
            defaultValue = static_cast<std::uint32_t>(std::stoul(stringValue));
            defaultSet = true;
        }
        catch (...)
        {
        }

        // Create the mapping
        if (defaultSet)
        {
            mapping = std::make_shared<UInt32Mapping>(
              jsonMapping.getReference(), jsonMapping.getRegisterType(),
              std::vector<std::int32_t>{static_cast<short>(jsonMapping.getAddress()),
                                        static_cast<short>(jsonMapping.getAddress() + 1)},
              jsonMapping.getOperationType(), jsonMapping.isReadRestricted(), -1, jsonMapping.getDeadbandValue(),
              jsonMapping.getFrequencyFilterValue(), jsonMapping.getRepeat(), &defaultValue);
        }
        else
        {
            // Make the mapping
            mapping = std::make_shared<UInt32Mapping>(
              jsonMapping.getReference(), jsonMapping.getRegisterType(),
              std::vector<std::int32_t>{static_cast<short>(jsonMapping.getAddress()),
                                        static_cast<short>(jsonMapping.getAddress() + 1)},
              jsonMapping.getOperationType(), jsonMapping.isReadRestricted(), -1, jsonMapping.getDeadbandValue(),
              jsonMapping.getFrequencyFilterValue(), jsonMapping.getRepeat());
        }
        return mapping;
    }
    case RegisterMapping::OutputType::INT32:
    {
        // Make place for the mapping
        auto mapping = std::shared_ptr<Int32Mapping>{};

        // Check if the default value is anything
        const auto& stringValue = jsonMapping.getDefaultValue();
        bool defaultSet = false;
        auto defaultValue = std::int32_t{};
        try
        {
            defaultValue = static_cast<std::int32_t>(std::stoi(stringValue));
            defaultSet = true;
        }
        catch (...)
        {
        }

        // Create the mapping
        if (defaultSet)
        {
            mapping = std::make_shared<Int32Mapping>(
              jsonMapping.getReference(), jsonMapping.getRegisterType(),
              std::vector<std::int32_t>{static_cast<short>(jsonMapping.getAddress()),
                                        static_cast<short>(jsonMapping.getAddress() + 1)},
              jsonMapping.getOperationType(), jsonMapping.isReadRestricted(), -1, jsonMapping.getDeadbandValue(),
              jsonMapping.getFrequencyFilterValue(), jsonMapping.getRepeat(), &defaultValue);
        }
        else
        {
            // Make the mapping
            mapping = std::make_shared<Int32Mapping>(
              jsonMapping.getReference(), jsonMapping.getRegisterType(),
              std::vector<std::int32_t>{static_cast<short>(jsonMapping.getAddress()),
                                        static_cast<short>(jsonMapping.getAddress() + 1)},
              jsonMapping.getOperationType(), jsonMapping.isReadRestricted(), -1, jsonMapping.getDeadbandValue(),
              jsonMapping.getFrequencyFilterValue(), jsonMapping.getRepeat());
        }
        return mapping;
    }
    case RegisterMapping::OutputType::FLOAT:
    {
        // Make place for the mapping
        auto mapping = std::shared_ptr<FloatMapping>{};

        // Check if the default value is anything
        const auto& stringValue = jsonMapping.getDefaultValue();
        bool defaultSet = false;
        float defaultValue;
        try
        {
            defaultValue = static_cast<float>(std::stof(stringValue));
            defaultSet = true;
        }
        catch (...)
        {
        }

        // Create the mapping
        if (defaultSet)
        {
            mapping = std::make_shared<FloatMapping>(
              jsonMapping.getReference(), jsonMapping.getRegisterType(),
              std::vector<std::int32_t>{static_cast<short>(jsonMapping.getAddress()),
                                        static_cast<short>(jsonMapping.getAddress() + 1)},
              jsonMapping.isReadRestricted(), -1, jsonMapping.getDeadbandValue(), jsonMapping.getFrequencyFilterValue(),
              jsonMapping.getRepeat(), &defaultValue);
        }
        else
        {
            // Make the mapping
            mapping = std::make_shared<FloatMapping>(
              jsonMapping.getReference(), jsonMapping.getRegisterType(),
              std::vector<std::int32_t>{static_cast<short>(jsonMapping.getAddress()),
                                        static_cast<short>(jsonMapping.getAddress() + 1)},
              jsonMapping.isReadRestricted(), -1, jsonMapping.getDeadbandValue(), jsonMapping.getFrequencyFilterValue(),
              jsonMapping.getRepeat());
        }
        return mapping;
    }
    case RegisterMapping::OutputType::STRING:
        std::vector<std::int32_t> addresses;
        const auto startAddress = static_cast<std::int32_t>(jsonMapping.getAddress());
        for (int i = 0; i < jsonMapping.getRegisterCount(); i++)
        {
            addresses.emplace_back(startAddress + i);
        }
        return std::make_shared<StringMapping>(jsonMapping.getReference(), jsonMapping.getRegisterType(), addresses,
                                               jsonMapping.getOperationType(), jsonMapping.isReadRestricted(), -1,
                                               jsonMapping.getFrequencyFilterValue(), jsonMapping.getRepeat(),
                                               jsonMapping.getDefaultValue());
    }
    return nullptr;
}
}    // namespace wolkabout
