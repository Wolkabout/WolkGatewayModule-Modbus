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

#ifndef WOLKGATEWAYMODBUSMODULE_KEYVALUEPERSISTENCE_H
#define WOLKGATEWAYMODBUSMODULE_KEYVALUEPERSISTENCE_H

#include <map>
#include <string>

namespace wolkabout
{
/**
 * This interface defines a means of persisting key-value pairs.
 */
class KeyValuePersistence
{
public:
    /**
     * Default virtual destructor.
     */
    virtual ~KeyValuePersistence() = default;

    /**
     * This is the method that allows the user to store a key value pair in the persistence.
     *
     * @param key The key under which the value will be stored.
     * @param value The value that needs to be stored.
     * @return Whether the pair was successfully stored.
     */
    virtual bool storeValue(const std::string& key, const std::string& value) = 0;

    /**
     * This is the method that allows the user to load the values.
     *
     * @return The map containing all key-value pair this persistence had stored.
     */
    virtual std::map<std::string, std::string> loadValues() = 0;
};
}    // namespace wolkabout

#endif    // WOLKGATEWAYMODBUSMODULE_KEYVALUEPERSISTENCE_H
