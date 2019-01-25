/*
 * Copyright 2019 WolkAbout Technology s.r.o.
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

#include "ModbusRegisterGroup.h"
#include "ModbusRegisterMapping.h"

#include <algorithm>
#include <string>
#include <vector>

namespace wolkabout
{
ModbusRegisterGroup::ModbusRegisterGroup(int slaveAddress, ModbusRegisterMapping::RegisterType registerType,
                                         ModbusRegisterMapping::DataType dataType)
: m_slaveAddress(slaveAddress)
, m_registerType(registerType)
, m_dataType(dataType)
{
    std::vector<wolkabout::ModbusRegisterMapping> m_modbusRegisterMappings;
}

int ModbusRegisterGroup::getSlaveAddress()
{
    return m_slaveAddress;
}

ModbusRegisterMapping::RegisterType ModbusRegisterGroup::getRegisterType()
{
    return m_registerType;
}

ModbusRegisterMapping::DataType ModbusRegisterGroup::getDataType()
{
    return m_dataType;
}

bool ModbusRegisterGroup::getStartingRegisterAddress(int& address)
{
    if (m_modbusRegisterMappings.size() == 0)
    {
        return false;
    }
    address = m_modbusRegisterMappings.front().getAddress();
    return true;
}

int ModbusRegisterGroup::getRegisterCount()
{
    return m_modbusRegisterMappings.size();
}

void ModbusRegisterGroup::addRegister(ModbusRegisterMapping modbusRegisterMapping)
{
    if (m_modbusRegisterMappings.size() == 0)
    {
        m_modbusRegisterMappings.push_back(modbusRegisterMapping);
        return;
    }
    if (modbusRegisterMapping.getAddress() == m_modbusRegisterMappings.front().getAddress() - 1)
    {
        m_modbusRegisterMappings.insert(m_modbusRegisterMappings.front(), modbusRegisterMapping);
        return;
    }
    if (modbusRegisterMapping.getAddress() == m_modbusRegisterMappings.back().getAddress() + 1)
    {
        m_modbusRegisterMappings.push_back(modbusRegisterMapping);
        return;
    }
}

}    // namespace wolkabout

#endif
