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

#include "module/SafeModeValuePersistence.h"

#include "core/utilities/FileSystemUtils.h"
#include "core/utilities/Logger.h"
#include "core/utilities/json.hpp"

using namespace nlohmann;

namespace wolkabout
{
const std::string JSON_PATH = "./safe-mode.json";
const std::string SEPARATOR = ".";

bool SafeModeValuePersistence::storeSafeModeValue(const std::string& deviceKey, const std::string& reference,
                                                  std::string value)
{
    LOG(TRACE) << METHOD_INFO;

    // Check if the file exists
    auto j = json{};
    if (FileSystemUtils::isFilePresent(JSON_PATH))
    {
        // Read the content of the file
        auto content = std::string{};
        if (FileSystemUtils::readFileContent(JSON_PATH, content))
        {
            try
            {
                j = json::parse(content);
            }
            catch (...)
            {
            }
        }
    }

    // Now change the value in the json
    j[deviceKey + SEPARATOR + reference] = value;

    // And now try to write it in
    return FileSystemUtils::createFileWithContent(JSON_PATH, j.dump(4));
}

std::map<std::string, std::map<std::string, std::string>> SafeModeValuePersistence::loadSafeModeValues()
{
    LOG(TRACE) << METHOD_INFO;

    // Make place for the map that is being read.
    auto map = std::map<std::string, std::map<std::string, std::string>>{};

    // Now try to read the file
    if (FileSystemUtils::isFilePresent(JSON_PATH))
    {
        // Make place for the file content
        auto content = std::string{};
        if (FileSystemUtils::readFileContent(JSON_PATH, content))
        {
            try
            {
                // Parse the JSON
                auto j = json::parse(content);
                for (const auto& pair : j.items())
                {
                    // Split the key into type and reference
                    const auto& key = pair.key();
                    const auto deviceType = key.substr(0, key.find(SEPARATOR));
                    const auto reference = key.substr(key.find(SEPARATOR) + 1);

                    // Check if the map already has an entry for the device type
                    if (map.find(deviceType) != map.cend())
                        map[deviceType].emplace(reference, pair.value());
                    else
                        map.emplace(deviceType, std::map<std::string, std::string>{{reference, pair.value()}});
                }
            }
            catch (...)
            {
            }
        }
    }

    // Return the map in any way shape or form we have it
    return map;
}
}    // namespace wolkabout
