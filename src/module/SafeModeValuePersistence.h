/**
 * Copyright 2021 Wolkabout s.r.o.
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

#ifndef WOLKGATEWAYMODBUSMODULE_SAFEMODEVALUEPERSISTENCE_H
#define WOLKGATEWAYMODBUSMODULE_SAFEMODEVALUEPERSISTENCE_H

#include <map>
#include <string>

namespace wolkabout
{
/**
 * This class represents persistence for safe mode values that have been changed by the platform.
 */
class SafeModeValuePersistence
{
public:
    /**
     * This method allows the user to change a safe mode value in persistence.
     *
     * @param deviceKey The name of the device type in which the mapping belongs.
     * @param reference The reference to the mapping which safe mode value needs changing.
     * @param value The new value for the safe mode of the mapping.
     * @return Whether the value was stored successfully.
     */
    bool storeSafeModeValue(const std::string& deviceKey, const std::string& reference, std::string value);

    /**
     * This method allows the user to read all the persisted safe mode values.
     *
     * @return The values that have been read from the json file.
     */
    std::map<std::string, std::map<std::string, std::string>> loadSafeModeValues();
};
}    // namespace wolkabout

#endif    // WOLKGATEWAYMODBUSMODULE_SAFEMODEVALUEPERSISTENCE_H
