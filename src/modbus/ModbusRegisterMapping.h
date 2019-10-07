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

#ifndef MODBUSREGISTERMAPPING_H
#define MODBUSREGISTERMAPPING_H

#include <string>
#include <vector>

namespace wolkabout
{
class ModbusRegisterMapping
{
public:
    enum class RegisterType
    {
        COIL = 0,
        INPUT_CONTACT,
        INPUT_REGISTER,
        HOLDING_REGISTER_SENSOR,
        HOLDING_REGISTER_ACTUATOR
    };

    enum class MappingType 
    {
        DEFAULT,
        SENSOR,
        ACTUATOR,
        ALARM,
        CONFIGURATION
    };

    enum class DataType
    {
        INT16 = 0,
        UINT16,
        REAL32,
        BOOL
    };

    ModbusRegisterMapping(std::string name, std::string reference, double minimum, double maximum, int address,
                          RegisterType registerType, DataType dataType, int slaveAddress, MappingType mappingType);

    const std::string& getName() const;
    const std::string& getReference() const;

    double getMinimum() const;
    double getMaximum() const;

    int getAddress() const;

    RegisterType getRegisterType() const;
    DataType getDataType() const;
    MappingType getMappingType() const;
    int getSlaveAddress() const;

private:
    std::string m_name;
    std::string m_reference;

    double m_minimum;
    double m_maximum;

    int m_address;

    RegisterType m_registerType;
    DataType m_dataType;
    MappingType m_mappingType;
    int m_slaveAddress;
};

class ModbusRegisterMappingFactory
{
public:
    static std::vector<wolkabout::ModbusRegisterMapping> fromJsonFile(const std::string& modbusRegisterMappingFile);

private:
    static ModbusRegisterMapping::RegisterType deserializeRegisterType(const std::string& registerType);
    static ModbusRegisterMapping::DataType deserializeDataType(const std::string& dataType);
    static ModbusRegisterMapping::MappingType deserializeMappingType(const std::string& mappingType);
};
}    // namespace wolkabout

#endif
