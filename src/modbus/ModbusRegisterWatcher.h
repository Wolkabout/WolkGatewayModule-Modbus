/*
 * Copyright 2018 WolkAbout Technology s.r.o.
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

#ifndef MODBUSREGISTERWATCHER_H
#define MODBUSREGISTERWATCHER_H

#include "modbus/ModbusRegisterMapping.h"

#include <string>

namespace wolkabout
{
class ModbusRegisterWatcher
{
public:
    ModbusRegisterWatcher();

    bool update(const std::string& newRegisterValue);

    bool update(signed short newRegisterValue);
    bool update(unsigned short newRegisterValue);
    bool update(float newRegisterValue);
    bool update(bool newRegisterValue);

    const std::string& getValue() const;

    void setValid(bool valid);

private:
    bool m_isInitialized;
    bool m_isValid;

    std::string m_value;
};
}    // namespace wolkabout

#endif
