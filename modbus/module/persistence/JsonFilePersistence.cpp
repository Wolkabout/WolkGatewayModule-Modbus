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

#include "modbus/module/persistence/JsonFilePersistence.h"

#include "core/utilities/FileSystemUtils.h"
#include "core/utilities/Logger.h"
#include "core/utilities/nlohmann/json.hpp"

#include <utility>

using namespace nlohmann;

namespace wolkabout
{
namespace modbus
{
JsonFilePersistence::JsonFilePersistence(std::string filePath) : m_filePath(std::move(filePath)) {}

bool JsonFilePersistence::storeValue(const std::string& key, const std::string& value)
{
    LOG(TRACE) << METHOD_INFO;

    // Check if the file exists
    json j;
    if (FileSystemUtils::isFilePresent(m_filePath))
    {
        // Read the content of the file
        auto content = std::string{};
        if (FileSystemUtils::readFileContent(m_filePath, content))
        {
            try
            {
                j = json::parse(content);
            }
            catch (const std::exception& exception)
            {
                LOG(WARN) << "Failed to load old data from '" << m_filePath
                          << "'. Old values will be ignored, and only new ones will be written in.";
            }
        }
    }

    // Now change the value in the json
    j[key] = value;

    // And now try to write it in
    return FileSystemUtils::createFileWithContent(m_filePath, j.dump(4));
}

std::map<std::string, std::string> JsonFilePersistence::loadValues()
{
    LOG(TRACE) << METHOD_INFO;

    // Make place for the map that is being read.
    auto map = std::map<std::string, std::string>{};

    // Now try to read the file
    if (!FileSystemUtils::isFilePresent(m_filePath))
    {
        LOG(WARN) << "Failed to load values from '" << m_filePath << "' -> The json file is not present.";
        return map;
    }

    // Make place for the file content
    auto content = std::string{};
    if (!FileSystemUtils::readFileContent(m_filePath, content))
    {
        LOG(WARN) << "Failed to load values from '" << m_filePath << "' -> Failed to read the content of the file.";
        return map;
    }

    try
    {
        // Parse the JSON
        auto j = json::parse(content);
        for (const auto& pair : j.items())
            map.emplace(pair.key(), pair.value().get<std::string>());
    }
    catch (const std::exception& exception)
    {
        LOG(ERROR) << "Failed to load values from '" << m_filePath << "' -> '" << exception.what() << "'.";
    }
    // Return the map in any way shape or form we have it
    return map;
}
}    // namespace modbus
}    // namespace wolkabout
