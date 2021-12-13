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

#ifndef WOLKGATEWAYMODBUSMODULE_JSONFILEPERSISTENCE_H
#define WOLKGATEWAYMODBUSMODULE_JSONFILEPERSISTENCE_H

#include "module/persistence/KeyValuePersistence.h"

#include <map>
#include <string>

namespace wolkabout
{
/**
 * This class represents persistent key value storage.
 */
class JsonFilePersistence : public KeyValuePersistence
{
public:
    /**
     * This is the default constructor for the persistence.
     *
     * @param filePath The path for the file in which the values will be stored.
     */
    explicit JsonFilePersistence(std::string filePath);

    /**
     * This method allows the user to store/modify a value in persistent storage.
     *
     * @param key The key under which the value will be placed
     * @param value The new value which will be stored.
     * @return Whether the value was stored successfully.
     */
    bool storeValue(const std::string& key, const std::string& value) override;

    /**
     * This method allows the user to read all the persisted key value pairs.
     *
     * @return The values that have been read from the json file.
     */
    std::map<std::string, std::string> loadValues() override;

private:
    // This is where we store the file path in which the values are placed/read from.
    std::string m_filePath;
};
}    // namespace wolkabout

#endif    // WOLKGATEWAYMODBUSMODULE_JSONFILEPERSISTENCE_H
