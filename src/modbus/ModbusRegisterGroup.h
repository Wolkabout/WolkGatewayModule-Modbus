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
    explicit ModbusRegisterGroup(const ModbusRegisterMapping& mapping);

    ModbusRegisterGroup(ModbusRegisterMapping::RegisterType registerType, ModbusRegisterMapping::DataType dataType);

    ModbusRegisterGroup(int slaveAddress, ModbusRegisterMapping::RegisterType registerType,
                        ModbusRegisterMapping::DataType dataType);

    ModbusRegisterGroup(const ModbusRegisterGroup& group);

    int getSlaveAddress() const;
    ModbusRegisterMapping::RegisterType getRegisterType() const;
    ModbusRegisterMapping::DataType getDataType() const;
    int getStartingRegisterAddress() const;
    int getRegisterCount() const;
    int getMappingsCount() const;
    std::vector<ModbusRegisterMapping> getRegisters() const;

    void addRegister(ModbusRegisterMapping modbusRegisterMapping);
    void setSlaveAddress(int slaveAddress);

private:
    int m_slaveAddress;
    ModbusRegisterMapping::RegisterType m_registerType;
    ModbusRegisterMapping::DataType m_dataType;
    std::vector<ModbusRegisterMapping> m_modbusRegisterMappings;
};
}    // namespace wolkabout

#endif
