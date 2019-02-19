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

#include <algorithm>

namespace wolkabout
{
ModbusRegisterGroup::ModbusRegisterGroup(int slaveAddress, ModbusRegisterMapping::RegisterType registerType,
                                         ModbusRegisterMapping::DataType dataType)
: m_slaveAddress(slaveAddress), m_registerType(registerType), m_dataType(dataType)
{
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

int ModbusRegisterGroup::getStartingRegisterAddress()
{
    return m_modbusRegisterMappings.front().getAddress();
}

int ModbusRegisterGroup::getRegisterCount()
{
    return m_modbusRegisterMappings.size();
}

const std::vector<ModbusRegisterMapping>& ModbusRegisterGroup::getRegisters() const
{
    return m_modbusRegisterMappings;
}

void ModbusRegisterGroup::addRegister(ModbusRegisterMapping modbusRegisterMapping)
{
    m_modbusRegisterMappings.push_back(modbusRegisterMapping);
}
}    // namespace wolkabout