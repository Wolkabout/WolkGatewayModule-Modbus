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

#include "ModbusBridge.h"
#include "ActuationHandler.h"
#include "ActuatorStatusProvider.h"
#include "DeviceStatusProvider.h"
#include "modbus/ModbusClient.h"
#include "modbus/ModbusConfiguration.h"
#include "modbus/ModbusRegisterMapping.h"
#include "utilities/Logger.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace wolkabout
{
ModbusBridge::ModbusBridge(ModbusClient& modbusClient, const std::vector<ModbusRegisterMapping>& modbusRegisterMappings,
                           std::chrono::duration<long long, std::milli> registerReadPeriod)
: m_modbusClient(modbusClient), m_registerReadPeriod(std::move(registerReadPeriod)), m_readerShouldRun(false)
{
    for (const ModbusRegisterMapping& modbusRegisterMapping : modbusRegisterMappings)
    {
        const std::string& reference = modbusRegisterMapping.getReference();

        m_referenceToModbusRegisterMapping.emplace(reference, modbusRegisterMapping);
        m_referenceToModbusRegisterWatcherMapping.emplace(reference, ModbusRegisterWatcher{});
    }
}

ModbusBridge::~ModbusBridge()
{
    m_modbusClient.disconnect();

    stop();
}

void ModbusBridge::onSensorReading(
  std::function<void(const std::string& reference, const std::string& value)> onSensorReading)
{
    m_onSensorReading = onSensorReading;
}

void ModbusBridge::onActuatorStatusChange(std::function<void(const std::string& reference)> onActuatorStatusChange)
{
    m_onActuatorStatusChange = onActuatorStatusChange;
}

void ModbusBridge::handleActuation(const std::string& /* deviceKey */, const std::string& reference,
                                   const std::string& value)
{
    LOG(INFO) << "ModbusBridge: Handling actuation for reference '" << reference << "'";
    if (m_referenceToModbusRegisterMapping.find(reference) == m_referenceToModbusRegisterMapping.end())
    {
        LOG(ERROR) << "ModbusBridge: No modbus register mapped to reference '" << reference << "'";
        return;
    }

    const ModbusRegisterMapping& modbusRegisterMapping = m_referenceToModbusRegisterMapping.at(reference);
    if (modbusRegisterMapping.getRegisterType() != ModbusRegisterMapping::RegisterType::HOLDING_REGISTER &&
        modbusRegisterMapping.getRegisterType() != ModbusRegisterMapping::RegisterType::COIL)
    {
        LOG(ERROR) << "ModbusBridge: Modbus register mapped to reference '" << reference
                   << "' can not be treated as actuator - Modbus register must be of type 'HOLDING_REGISTER' or 'COIL'";
        return;
    }

    if (modbusRegisterMapping.getRegisterType() == ModbusRegisterMapping::RegisterType::HOLDING_REGISTER)
    {
        handleActuationForHoldingRegister(modbusRegisterMapping, value);
    }
    else if (modbusRegisterMapping.getRegisterType() == ModbusRegisterMapping::RegisterType::COIL)
    {
        handleActuationForCoil(modbusRegisterMapping, value);
    }
}

wolkabout::ActuatorStatus ModbusBridge::getActuatorStatus(const std::string& /* deviceKey */,
                                                          const std::string& reference)
{
    LOG(INFO) << "ModbusBridge: Getting actuator status for reference '" << reference << "'";
    if (m_referenceToModbusRegisterMapping.find(reference) == m_referenceToModbusRegisterMapping.end())
    {
        LOG(ERROR) << "ModbusBridge: No modbus register mapped to reference '" << reference << "'";
        return ActuatorStatus("", ActuatorStatus::State::ERROR);
    }

    const ModbusRegisterMapping& modbusRegisterMapping = m_referenceToModbusRegisterMapping.at(reference);
    if (modbusRegisterMapping.getRegisterType() != ModbusRegisterMapping::RegisterType::HOLDING_REGISTER &&
        modbusRegisterMapping.getRegisterType() != ModbusRegisterMapping::RegisterType::COIL)
    {
        LOG(ERROR) << "ModbusBridge: Modbus register mapped to reference '" << reference
                   << "' can not be treated as actuator - Modbus register must be of type 'HOLDING_REGISTER' or 'COIL'";
        return ActuatorStatus("", ActuatorStatus::State::ERROR);
    }

    return modbusRegisterMapping.getRegisterType() == ModbusRegisterMapping::RegisterType::HOLDING_REGISTER ?
             getActuatorStatusFromHoldingRegister(modbusRegisterMapping) :
             getActuatorStatusFromCoil(modbusRegisterMapping);
}

wolkabout::DeviceStatus ModbusBridge::getStatus(const std::string& /* deviceKey */)
{
    return m_modbusClient.isConnected() ? DeviceStatus::CONNECTED : DeviceStatus::OFFLINE;
}

void ModbusBridge::start()
{
    if (m_readerShouldRun)
    {
        return;
    }

    m_modbusClient.connect();
    m_readerShouldRun = true;
    m_reader = std::unique_ptr<std::thread>(new std::thread(&ModbusBridge::run, this));
}

void ModbusBridge::stop()
{
    m_readerShouldRun = false;
    m_reader->join();
}

wolkabout::ActuatorStatus ModbusBridge::getActuatorStatusFromHoldingRegister(
  const wolkabout::ModbusRegisterMapping& modbusRegisterMapping) const
{
    if (modbusRegisterMapping.getDataType() == ModbusRegisterMapping::DataType::INT16)
    {
        signed short value;
        if (!m_modbusClient.readHoldingRegister(modbusRegisterMapping.getAddress(), value))
        {
            return ActuatorStatus("", ActuatorStatus::State::ERROR);
        }

        return ActuatorStatus(std::to_string(value), ActuatorStatus::State::READY);
    }
    else if (modbusRegisterMapping.getDataType() == ModbusRegisterMapping::DataType::UINT16)
    {
        unsigned short value;
        if (!m_modbusClient.readHoldingRegister(modbusRegisterMapping.getAddress(), value))
        {
            return ActuatorStatus("", ActuatorStatus::State::ERROR);
        }

        return ActuatorStatus(std::to_string(value), ActuatorStatus::State::READY);
    }
    else if (modbusRegisterMapping.getDataType() == ModbusRegisterMapping::DataType::REAL32)
    {
        float value;
        if (!m_modbusClient.readHoldingRegister(modbusRegisterMapping.getAddress(), value))
        {
            return ActuatorStatus("", ActuatorStatus::State::ERROR);
        }

        return ActuatorStatus(std::to_string(value), ActuatorStatus::State::READY);
    }

    LOG(ERROR) << "ModbusBridge: Could not obtain actuator status from holding register - Unsupported data type";
    return ActuatorStatus("", ActuatorStatus::State::ERROR);
}

wolkabout::ActuatorStatus ModbusBridge::getActuatorStatusFromCoil(
  const wolkabout::ModbusRegisterMapping& modbusRegisterMapping) const
{
    bool value;
    if (!m_modbusClient.readCoil(modbusRegisterMapping.getAddress(), value))
    {
        return ActuatorStatus("", ActuatorStatus::State::ERROR);
    }

    return ActuatorStatus(value ? "true" : "false", ActuatorStatus::State::READY);
}

void ModbusBridge::handleActuationForHoldingRegister(const wolkabout::ModbusRegisterMapping& modbusRegisterMapping,
                                                     const std::string& value) const
{
    if (modbusRegisterMapping.getDataType() == ModbusRegisterMapping::DataType::INT16)
    {
        signed short typedValue = static_cast<signed short>(std::atol(value.c_str()));
        if (!m_modbusClient.writeHoldingRegister(modbusRegisterMapping.getAddress(), typedValue))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << modbusRegisterMapping.getAddress() << " Value: " << value;
        }
    }
    else if (modbusRegisterMapping.getDataType() == ModbusRegisterMapping::DataType::UINT16)
    {
        unsigned short typedValue = static_cast<unsigned short>(std::stoul(value.c_str()));
        if (!m_modbusClient.writeHoldingRegister(modbusRegisterMapping.getAddress(), typedValue))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << modbusRegisterMapping.getAddress() << " Value: " << value;
        }
    }
    else if (modbusRegisterMapping.getDataType() == ModbusRegisterMapping::DataType::UINT16)
    {
        float typedValue = std::stof(value.c_str());
        if (!m_modbusClient.writeHoldingRegister(modbusRegisterMapping.getAddress(), typedValue))
        {
            LOG(ERROR) << "ModbusBridge: Unable to write holding register value - Register address: "
                       << modbusRegisterMapping.getAddress() << " Value: " << value;
        }
    }
}

void ModbusBridge::handleActuationForCoil(const wolkabout::ModbusRegisterMapping& modbusRegisterMapping,
                                          const std::string& value) const
{
    if (!m_modbusClient.writeCoil(modbusRegisterMapping.getAddress(), value == "true" ? true : false))
    {
        LOG(ERROR) << "ModbusBridge: Unable to write coil value - Register address: "
                   << modbusRegisterMapping.getAddress() << " Value: " << value;
    }
}

void ModbusBridge::run()
{
    while (m_readerShouldRun)
    {
        readAndReportModbusRegistersValues();

        std::this_thread::sleep_for(m_registerReadPeriod);
    }
}

void ModbusBridge::readAndReportModbusRegistersValues()
{
    LOG(DEBUG) << "ModbusBridge: Reading and reporting register values";

    for (const auto& it : m_referenceToModbusRegisterMapping)
    {
        const std::string& reference = it.first;
        if (m_referenceToModbusRegisterWatcherMapping.find(reference) ==
            m_referenceToModbusRegisterWatcherMapping.cend())
        {
            m_referenceToModbusRegisterWatcherMapping.emplace(reference, ModbusRegisterWatcher{});
        }

        const ModbusRegisterMapping& modbusRegisterMapping = it.second;
        ModbusRegisterWatcher& modbusRegisterWatcher = m_referenceToModbusRegisterWatcherMapping.at(reference);

        if (!isRegisterValueUpdated(modbusRegisterMapping, modbusRegisterWatcher))
        {
            continue;
        }

        if (modbusRegisterMapping.getRegisterType() == ModbusRegisterMapping::RegisterType::HOLDING_REGISTER ||
            modbusRegisterMapping.getRegisterType() == ModbusRegisterMapping::RegisterType::COIL)
        {
            LOG(INFO) << "ModbusBridge: Actuator value changed - Reference: '" << modbusRegisterMapping.getReference()
                      << "' Value: '" << modbusRegisterWatcher.getValue() << "'";

            if (m_onActuatorStatusChange)
            {
                m_onActuatorStatusChange(modbusRegisterMapping.getReference());
            }
        }

        if (modbusRegisterMapping.getRegisterType() == ModbusRegisterMapping::RegisterType::INPUT_REGISTER ||
            modbusRegisterMapping.getRegisterType() == ModbusRegisterMapping::RegisterType::INPUT_BIT)
        {
            LOG(INFO) << "ModbusBridge: Sensor value - Reference: '" << modbusRegisterMapping.getReference()
                      << "' Value: '" << modbusRegisterWatcher.getValue() << "'";

            if (m_onSensorReading)
            {
                m_onSensorReading(modbusRegisterMapping.getReference(), modbusRegisterWatcher.getValue());
            }
        }
    }
}

bool ModbusBridge::isRegisterValueUpdated(const ModbusRegisterMapping& modbusRegisterMapping,
                                          ModbusRegisterWatcher& modbusRegisterWatcher)
{
    switch (modbusRegisterMapping.getRegisterType())
    {
    case ModbusRegisterMapping::RegisterType::HOLDING_REGISTER:
        return isHoldingRegisterValueUpdated(modbusRegisterMapping, modbusRegisterWatcher);

    case ModbusRegisterMapping::RegisterType::INPUT_REGISTER:
        return isInputRegisterValueUpdated(modbusRegisterMapping, modbusRegisterWatcher);

    case ModbusRegisterMapping::RegisterType::COIL:
        return isCoilValueUpdated(modbusRegisterMapping, modbusRegisterWatcher);

    case ModbusRegisterMapping::RegisterType::INPUT_BIT:
        return isInputBitValueUpdated(modbusRegisterMapping, modbusRegisterWatcher);
    }

    LOG(ERROR) << "ModbusBridge: isRegisterValueUpdated - Unhandled register type for reference'"
               << modbusRegisterMapping.getReference() << "'";
    assert(false && "ModbusBridge: isRegisterValueUpdated - Unhandled register type");
    return false;
}

bool ModbusBridge::isHoldingRegisterValueUpdated(const ModbusRegisterMapping& modbusRegisterMapping,
                                                 ModbusRegisterWatcher& modbusRegisterWatcher)
{
    switch (modbusRegisterMapping.getDataType())
    {
    case ModbusRegisterMapping::DataType::INT16:
    {
        signed short valueShort;
        if (!m_modbusClient.readHoldingRegister(modbusRegisterMapping.getAddress(), valueShort))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read holding register with address '"
                       << modbusRegisterMapping.getAddress() << "'";
            return false;
        }

        return modbusRegisterWatcher.update(valueShort);
    }

    case ModbusRegisterMapping::DataType::UINT16:
    {
        unsigned short valueUnsignedShort;
        if (!m_modbusClient.readHoldingRegister(modbusRegisterMapping.getAddress(), valueUnsignedShort))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read holding register with address '"
                       << modbusRegisterMapping.getAddress() << "'";
            return false;
        }

        return modbusRegisterWatcher.update(valueUnsignedShort);
    }

    case ModbusRegisterMapping::DataType::REAL32:
    {
        float valueFloat;
        if (!m_modbusClient.readHoldingRegister(modbusRegisterMapping.getAddress(), valueFloat))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read holding register with address '"
                       << modbusRegisterMapping.getAddress() << "'";
            return false;
        }

        return modbusRegisterWatcher.update(valueFloat);
    }

    case ModbusRegisterMapping::DataType::BOOL:
    {
        signed short valueShort;
        if (!m_modbusClient.readHoldingRegister(modbusRegisterMapping.getAddress(), valueShort))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read holding register with address '"
                       << modbusRegisterMapping.getAddress() << "'";
            return false;
        }

        return modbusRegisterWatcher.update(valueShort != 0 ? true : false);
    }
    }

    LOG(ERROR) << "ModbusBridge: isHoldingRegisterValueUpdated - Unhandled data type for reference'"
               << modbusRegisterMapping.getReference() << "'";
    assert(false && "ModbusBridge: isHoldingRegisterValueUpdated - Unhandled data type");
    return false;
}

bool ModbusBridge::isInputRegisterValueUpdated(const ModbusRegisterMapping& modbusRegisterMapping,
                                               ModbusRegisterWatcher& modbusRegisterWatcher)
{
    switch (modbusRegisterMapping.getDataType())
    {
    case ModbusRegisterMapping::DataType::INT16:
    {
        signed short valueShort;
        if (!m_modbusClient.readInputRegister(modbusRegisterMapping.getAddress(), valueShort))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read holding register with address '"
                       << modbusRegisterMapping.getAddress() << "'";
            return false;
        }

        return modbusRegisterWatcher.update(valueShort);
    }

    case ModbusRegisterMapping::DataType::UINT16:
    {
        unsigned short valueUnsignedShort;
        if (!m_modbusClient.readInputRegister(modbusRegisterMapping.getAddress(), valueUnsignedShort))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read holding register with address '"
                       << modbusRegisterMapping.getAddress() << "'";
            return false;
        }

        return modbusRegisterWatcher.update(valueUnsignedShort);
    }

    case ModbusRegisterMapping::DataType::REAL32:
    {
        float valueFloat;
        if (!m_modbusClient.readInputRegister(modbusRegisterMapping.getAddress(), valueFloat))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read holding register with address '"
                       << modbusRegisterMapping.getAddress() << "'";
            return false;
        }

        return modbusRegisterWatcher.update(valueFloat);
    }

    case ModbusRegisterMapping::DataType::BOOL:
    {
        signed short valueShort;
        if (!m_modbusClient.readInputRegister(modbusRegisterMapping.getAddress(), valueShort))
        {
            LOG(ERROR) << "ModbusBridge: Unable to read input register with address '"
                       << modbusRegisterMapping.getAddress() << "'";
            return false;
        }

        return modbusRegisterWatcher.update(valueShort != 0 ? true : false);
    }
    }

    LOG(ERROR) << "ModbusBridge: isInputRegisterValueUpdated - Unhandled data type for reference'"
               << modbusRegisterMapping.getReference() << "'";
    assert(false && "ModbusBridge: isInputRegisterValueUpdated - Unhandled data type");
    return false;
}

bool ModbusBridge::isCoilValueUpdated(const ModbusRegisterMapping& modbusRegisterMapping,
                                      ModbusRegisterWatcher& modbusRegisterWatcher)
{
    bool value;
    if (!m_modbusClient.readCoil(modbusRegisterMapping.getAddress(), value))
    {
        LOG(ERROR) << "ModbusBridge: Unable to read coil with address '" << modbusRegisterMapping.getAddress() << "'";
        return false;
    }

    return modbusRegisterWatcher.update(value);
}

bool ModbusBridge::isInputBitValueUpdated(const ModbusRegisterMapping& modbusRegisterMapping,
                                          ModbusRegisterWatcher& modbusRegisterWatcher)
{
    bool value;
    if (!m_modbusClient.readInputBit(modbusRegisterMapping.getAddress(), value))
    {
        LOG(ERROR) << "ModbusBridge: Unable to read input bit with address '" << modbusRegisterMapping.getAddress()
                   << "'";
        return false;
    }

    return modbusRegisterWatcher.update(value);
}
}    // namespace wolkabout