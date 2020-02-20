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
    ModbusRegisterWatcher(int slaveAddress, const ModbusRegisterMapping& mapping);

    bool update(const std::string& newRegisterValue);
    bool update(signed short newRegisterValue);
    bool update(unsigned short newRegisterValue);
    bool update(float newRegisterValue);
    bool update(bool newRegisterValue);
    bool update(const std::vector<bool>& newRegisterValue);
    bool update(const std::vector<short>& newRegisterValue);
    bool update(const std::vector<unsigned short>& newRegisterValue);
    bool update(const std::vector<float>& newRegisterValue);

    const std::string& getValue() const;

    int getSlaveAddress() const;

    const ModbusRegisterMapping& getMapping();

    void setValid(bool valid);

private:
    bool m_isInitialized;
    bool m_isValid;

    std::string m_value;

    int m_slaveAddress;
    ModbusRegisterMapping& m_mapping;
};
}    // namespace wolkabout

#endif
