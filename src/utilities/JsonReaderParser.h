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

#include "utilities/FileSystemUtils.h"
#include "utilities/json.hpp"

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
     * @brief Read field of object as string, if it doesn't exist, return the default value.
     * @param object json with values
     * @param key being used for query
     * @param defaultValue value that will be returned if json misses key, of type string
     * @return value from json as string, or default value if key couldn't be found in json
     */
    static std::string readOrDefault(json::object_t object, const std::string& key, std::string defaultValue)
    {
        try
        {
            return object.at(key).get<std::string>();
        }
        catch (std::exception& e)
        {
            return defaultValue;
        }
    }

    /**
     * @brief Read field of object as double, if it doesn't exist, return the default value.
     * @param object json with values
     * @param key being used for query
     * @param defaultValue value that will be returned if json misses key, of type double
     * @return value from json as double, or default value if key couldn't be found in json
     */
    static double readOrDefault(json::object_t object, const std::string& key, double defaultValue)
    {
        try
        {
            return object.at(key).get<double>();
        }
        catch (std::exception& e)
        {
            return defaultValue;
        }
    }

    /**
     * @brief Read field of object as int, if it doesn't exist, return the default value.
     * @param object json with values
     * @param key being used for query
     * @param defaultValue value that will be returned if json misses key, of type int
     * @return value from json as int, or default value if key couldn't be found in json
     */
    static int readOrDefault(json::object_t object, const std::string& key, int defaultValue)
    {
        try
        {
            return object.at(key).get<int>();
        }
        catch (std::exception& e)
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
        if (!FileSystemUtils::isFilePresent(path))
        {
            throw std::logic_error("Given file does not exist (" + path + ").");
        }

        std::string jsonString;
        if (!FileSystemUtils::readFileContent(path, jsonString))
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
