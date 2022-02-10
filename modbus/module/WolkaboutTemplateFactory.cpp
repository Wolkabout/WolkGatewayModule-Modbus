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

#include "modbus/module/WolkaboutTemplateFactory.h"

namespace wolkabout
{
namespace modbus
{
std::unique_ptr<wolkabout::DeviceRegistrationData>
WolkaboutTemplateFactory::makeRegistrationDataFromDeviceConfigTemplate(const DeviceTemplate& configTemplate)
{
    // Make place for the feeds and attributes
    auto feeds = std::map<std::string, Feed>{};
    auto attributes = std::map<std::string, Attribute>{};

    // Go through all the mappings
    for (const ModuleMapping& mapping : configTemplate.getMappings())
    {
        const auto mappingType = mapping.getMappingType();
        const auto dataType = getDataTypeFromMapping(mapping);

        if (mappingType == MappingType::Attribute)
        {
            attributes.emplace(mapping.getReference(),
                               Attribute{mapping.getName(), dataType, mapping.getDefaultValue()});
        }
        else
        {
            feeds.emplace(mapping.getReference(), Feed{mapping.getName(), mapping.getReference(),
                                                       getFeedTypeFromMapping(mapping), toString(dataType)});
        }

        // Create special feeds
        if (mapping.getRepeat().count() > 0)
        {
            auto repeatFeed = Feed{"RepeatedWrite of " + mapping.getName(), "RPW(" + mapping.getReference() + ")",
                                   FeedType::IN_OUT, toString(DataType::NUMERIC)};
            feeds.emplace(repeatFeed.getReference(), std::move(repeatFeed));
        }
        if (mapping.hasSafeMode())
        {
            auto safeModeFeed = Feed{"SafeModeValue of " + mapping.getName(), "SMV(" + mapping.getReference() + ")",
                                     FeedType::IN_OUT, toString(dataType)};
            feeds.emplace(safeModeFeed.getReference(), std::move(safeModeFeed));
        }
        if (!mapping.getDefaultValue().empty())
        {
            auto defaultValueFeed = Feed{"DefaultValue of " + mapping.getName(), "DFV(" + mapping.getReference() + ")",
                                         FeedType::IN_OUT, toString(dataType)};
            feeds.emplace(defaultValueFeed.getReference(), std::move(defaultValueFeed));
        }
    }

    // Return the empty DeviceRegistrationData value with the generated feeds and attributes.
    return std::unique_ptr<DeviceRegistrationData>(new DeviceRegistrationData{"", "", "", {}, feeds, attributes});
}

DataType WolkaboutTemplateFactory::getDataTypeFromMapping(const ModuleMapping& mapping)
{
    if (mapping.getRegisterType() == more_modbus::RegisterType::COIL ||
        mapping.getRegisterType() == more_modbus::RegisterType::INPUT_CONTACT)
    {
        return DataType::BOOLEAN;
    }
    else
    {
        if (mapping.getOperationType() == more_modbus::OperationType::TAKE_BIT)
            return DataType::BOOLEAN;
        else if (mapping.getOperationType() == more_modbus::OperationType::STRINGIFY_UNICODE ||
                 mapping.getOperationType() == more_modbus::OperationType::STRINGIFY_ASCII)
            return DataType::STRING;
        return DataType::NUMERIC;
    }
}

FeedType WolkaboutTemplateFactory::getFeedTypeFromMapping(const ModuleMapping& mapping)
{
    switch (mapping.getMappingType())
    {
    case MappingType::Default:
        switch (mapping.getRegisterType())
        {
        case more_modbus::RegisterType::HOLDING_REGISTER:
        case more_modbus::RegisterType::COIL:
            return FeedType::IN_OUT;
        default:
            return FeedType::IN;
        }
    case MappingType::ReadWrite:
    case MappingType::WriteOnly:
        return FeedType::IN_OUT;
    default:
        return FeedType::IN;
    }
}
}    // namespace modbus
}    // namespace wolkabout
