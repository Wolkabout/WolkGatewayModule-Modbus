//
// Created by nvuletic on 2/18/20.
//

#include "JsonReaderParser.h"

namespace wolkabout
{
json::object_t JsonReaderParser::readFile(const std::string& path)
{
    if (!FileSystemUtils::isFilePresent(path))
    {
        throw std::logic_error("Given file does not exist (" + path + ").");
    }

    std::string jsonString;
    if (!FileSystemUtils::readFileContent(path, jsonString))
    {
        throw std::logic_error("Unable to read file ( " + path + " ).");
    }

    try
    {
        return json::parse(jsonString);
    }
    catch (std::exception&)
    {
        throw std::logic_error("Unable to parse file ( " + path + " ).");
    }
}
}    // namespace wolkabout
