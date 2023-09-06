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

#ifndef WOLKGATEWAYMODBUSMODULE_JSONREADERPARSER_H
#define WOLKGATEWAYMODBUSMODULE_JSONREADERPARSER_H

#include "core/utilities/FileSystemUtils.h"
#include <nlohmann/json.hpp>

namespace wolkabout
{
using nlohmann::json;

/**
 * @brief Collection of methods used for parsing data from JSON files.
 */
class JsonReaderParser
{
public:
    /**
     * This is the method that will attempt to read a value from a json object. If it fails, it will return a readable
     * exception that should be output to the user.
     *
     * @tparam T The type of the value being read from the json object.
     * @param object The object from which the value is being read.
     * @param key The key under which the value can be found.
     * @return The read value.
     */
    template <class T> static T read(json::object_t object, const std::string& key)
    {
        try
        {
            return object.at(key).get<T>();
        }
        catch (const std::exception&)
        {
            throw std::runtime_error("Failed to read field '" + key + "' of JSON object.");
        }
    }

    static std::string readTypedValue(json::object_t object, const std::string& key)
    {
        try
        {
            if (object.find(key) == object.cend())
                return {};
            switch (object.at(key).type())
            {
            case json::value_t::number_integer:
            case json::value_t::number_unsigned:
                return std::to_string(JsonReaderParser::readOrDefault(object, key, 0));
            case json::value_t::number_float:
                return std::to_string(JsonReaderParser::readOrDefault(object, key, double(0.0)));
            case json::value_t::boolean:
                return object.at(key).get<bool>() ? "true" : "false";
            default:
                return JsonReaderParser::readOrDefault(object, key, std::string{});
            }
        }
        catch (const std::exception&)
        {
            throw std::runtime_error("Failed to read '" + key + "'.");
        }
    }

    /**
     * This is the method that will attempt to read a value from a json object. If it fails, it will just return the
     * default value.
     *
     * @tparam T The type of the value being read from the json object.
     * @param object The object from which the value is being read.
     * @param key The key under which the value can be found.
     * @param defaultValue The default value that will be returned in case the value does not exist.
     * @return The read value or default value.
     */
    template <class T> static T readOrDefault(json::object_t object, const std::string& key, T defaultValue)
    {
        try
        {
            return object.at(key).get<T>();
        }
        catch (const std::exception&)
        {
            return defaultValue;
        }
    }

    /**
     * @brief Read file and return it as a json object.
     * @details Check for the files existence, if we're able to read it, and then parse it.
     * @param path string to file location
     * @return object parsed as json
     */
    static json::object_t readFile(const std::string& path)
    {
        if (!legacy::FileSystemUtils::isFilePresent(path))
        {
            throw std::logic_error("Given file does not exist (" + path + ").");
        }

        std::string jsonString;
        if (!legacy::FileSystemUtils::readFileContent(path, jsonString))
        {
            throw std::logic_error("Unable to read file (" + path + ").");
        }

        try
        {
            return json::parse(jsonString);
        }
        catch (std::exception&)
        {
            throw std::logic_error("Unable to parse file (" + path + ").");
        }
    }
};
}    // namespace wolkabout
#endif    // WOLKGATEWAYMODBUSMODULE_JSONREADERPARSER_H
