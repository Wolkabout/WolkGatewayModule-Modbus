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

#ifndef MODBUSBRIDGE_H
#define MODBUSBRIDGE_H

#include "ActuationHandler.h"
#include "ActuatorStatusProvider.h"
#include "DeviceStatusProvider.h"
#include "modbus/ModbusRegisterWatcher.h"

#include <atomic>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace wolkabout
{
class ModbusRegisterMapping;
class ModbusClient;

class ModbusBridge : public ActuationHandler, ActuatorStatusProvider, DeviceStatusProvider
{
public:
    ModbusBridge(ModbusClient& modbusClient, const std::vector<ModbusRegisterMapping>& modbusRegisterMappings,
                 std::chrono::duration<long long, std::milli> registerReadPeriod);

    virtual ~ModbusBridge();

    void onSensorReading(std::function<void(const std::string& reference, const std::string& value)> onSensorReading);
    void onActuatorStatusChange(std::function<void(const std::string& reference)> onActuatorStatusChange);

    void stop();

protected:
    void handleActuation(const std::string& deviceKey, const std::string& reference, const std::string& value) override;

    ActuatorStatus getActuatorStatus(const std::string& deviceKey, const std::string& reference) override;

    DeviceStatus getStatus(const std::string& deviceKey) override;

private:
    ActuatorStatus getActuatorStatusFromHoldingRegister(const ModbusRegisterMapping& modbusRegisterMapping) const;
    ActuatorStatus getActuatorStatusFromCoil(const ModbusRegisterMapping& modbusRegisterMapping) const;

    void handleActuationForHoldingRegister(const ModbusRegisterMapping& modbusRegisterMapping,
                                           const std::string& value) const;
    void handleActuationForCoil(const ModbusRegisterMapping& modbusRegisterMapping, const std::string& value) const;

    void run();

    void readAndReportModbusRegistersValues();

    bool isRegisterValueUpdated(const ModbusRegisterMapping& modbusRegisterMapping,
                                ModbusRegisterWatcher& modbusRegisterWatcher);

    bool isHoldingRegisterValueUpdated(const ModbusRegisterMapping& modbusRegisterMapping,
                                       ModbusRegisterWatcher& modbusRegisterWatcher);
    bool isInputRegisterValueUpdated(const ModbusRegisterMapping& modbusRegisterMapping,
                                     ModbusRegisterWatcher& modbusRegisterWatcher);
    bool isCoilValueUpdated(const ModbusRegisterMapping& modbusRegisterMapping,
                            ModbusRegisterWatcher& modbusRegisterWatcher);
    bool isInputBitValueUpdated(const ModbusRegisterMapping& modbusRegisterMapping,
                                ModbusRegisterWatcher& modbusRegisterWatcher);

    ModbusClient& m_modbusClient;

    std::map<std::string, ModbusRegisterMapping> m_referenceToModbusRegisterMapping;
    std::map<std::string, ModbusRegisterWatcher> m_referenceToModbusRegisterWatcherMapping;

    std::chrono::duration<long long, std::milli> m_registerReadPeriod;

    std::atomic_bool m_readerShouldRun;
    std::unique_ptr<std::thread> m_reader;

    std::function<void(const std::string& reference, const std::string& value)> m_onSensorReading;
    std::function<void(const std::string& reference)> m_onActuatorStatusChange;
};
}    // namespace wolkabout

#endif
