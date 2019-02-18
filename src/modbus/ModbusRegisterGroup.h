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

#ifndef MODBUSREGISTERGROUP_H
#define MODBUSREGISTERGROUP_H

#include "modbus/ModbusRegisterMapping.h"

#include <vector>

namespace wolkabout
{
class ModbusRegisterGroup
{
public:
    ModbusRegisterGroup(int slaveAddress, ModbusRegisterMapping::RegisterType registerType,
                        ModbusRegisterMapping::DataType dataType);

    int getSlaveAddress();
    ModbusRegisterMapping::RegisterType getRegisterType();
    ModbusRegisterMapping::DataType getDataType();
    int getStartingRegisterAddress();
    int getRegisterCount();
    void addRegister(ModbusRegisterMapping modbusRegisterMapping);
    const std::vector<ModbusRegisterMapping>& getRegisters() const;

private:
    int m_slaveAddress;
    ModbusRegisterMapping::RegisterType m_registerType;
    ModbusRegisterMapping::DataType m_dataType;
    std::vector<ModbusRegisterMapping> m_modbusRegisterMappings;
};
}    // namespace wolkabout

#endif
