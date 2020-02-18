//
// Created by nvuletic on 2/18/20.
//

#ifndef WOLKGATEWAYMODBUSMODULE_JSONREADERPARSER_H
#define WOLKGATEWAYMODBUSMODULE_JSONREADERPARSER_H

#include "utilities/FileSystemUtils.h"
#include "utilities/json.hpp"

namespace wolkabout
{
using nlohmann::json;

class JsonReaderParser
{
public:
    static json::object_t readFile(const std::string& path);
};
}    // namespace wolkabout
#endif    // WOLKGATEWAYMODBUSMODULE_JSONREADERPARSER_H
