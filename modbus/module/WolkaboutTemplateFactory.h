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

#ifndef WOLKGATEWAYMODBUSMODULE_WOLKABOUTTEMPLATEFACTORY_H
#define WOLKGATEWAYMODBUSMODULE_WOLKABOUTTEMPLATEFACTORY_H

#include "core/Types.h"
#include "core/model/messages/DeviceRegistrationMessage.h"
#include "modbus/model/DeviceTemplate.h"
#include "more_modbus/RegisterMapping.h"

namespace wolkabout
{
namespace modbus
{
/**
 * @brief Collection of utility methods used to create a DeviceTemplate (class used
 *        to register devices on the platform) from ModuleMappings, read from JSON.
 */
class WolkaboutTemplateFactory
{
public:
    /**
     * @brief Return the entire DeviceTemplate necessary for the Wolk instance to register the device.
     * @param configTemplate template passed from the configuration file.
     * @return DeviceTemplate ready for Wolk to register devices.
     */
    static std::unique_ptr<DeviceRegistrationData> makeRegistrationDataFromDeviceConfigTemplate(
      const DeviceTemplate& configTemplate);

private:
    /**
     * @brief Return the default data type for the mapping.
     * @param mapping The mapping for which the data type needs to be obtained.
     * @return Data type that is appropriate for this mapping.
     */
    static DataType getDataTypeFromMapping(const ModuleMapping& mapping);

    /**
     * @brief Return the default feed type for the mapping.
     * @param mapping The mapping for which the feed type needs to be obtained.
     * @return Feed type that is appropriate for this mapping.
     */
    static FeedType getFeedTypeFromMapping(const ModuleMapping& mapping);
};
}    // namespace modbus
}    // namespace wolkabout

#endif    // WOLKGATEWAYMODBUSMODULE_WOLKABOUTTEMPLATEFACTORY_H
