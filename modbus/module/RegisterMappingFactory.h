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

#ifndef WOLKGATEWAYMODBUSMODULE_REGISTERMAPPINGFACTORY_H
#define WOLKGATEWAYMODBUSMODULE_REGISTERMAPPINGFACTORY_H

#include "modbus/model/ModuleMapping.h"

namespace wolkabout
{
namespace modbus
{
/**
 * @brief Collection of method that create RegisterMappings for the MoreModbus library
 *        from the JSON mapping.
 */
class RegisterMappingFactory
{
public:
    /**
     * @brief Create a RegisterMapping from a JSON mapping.
     * @param jsonMapping parsed json from the config file.
     * @return created instance of RegisterMapping.
     */
    static std::shared_ptr<more_modbus::RegisterMapping> fromJSONMapping(const ModuleMapping& jsonMapping);
};
}    // namespace modbus
}    // namespace wolkabout

#endif    // WOLKGATEWAYMODBUSMODULE_REGISTERMAPPINGFACTORY_H
