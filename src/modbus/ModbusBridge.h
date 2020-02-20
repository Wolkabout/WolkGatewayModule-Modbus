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

#include "ActuationHandlerPerDevice.h"
#include "ActuatorStatusProviderPerDevice.h"
#include "ConfigurationHandlerPerDevice.h"
#include "ConfigurationProviderPerDevice.h"
#include "DeviceStatusProvider.h"
#include "modbus/ModbusRegisterGroup.h"
#include "modbus/ModbusRegisterWatcher.h"
#include "model/ConfigurationItem.h"
#include "model/Device.h"
#include "module/DeviceInformation.h"
#include "module/DevicesConfiguration.h"
#include "module/DevicesConfigurationTemplate.h"

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

class ModbusBridge : public ActuationHandlerPerDevice,
                     public ActuatorStatusProviderPerDevice,
                     public ConfigurationHandlerPerDevice,
                     public ConfigurationProviderPerDevice,
                     public DeviceStatusProvider
{
public:
    ModbusBridge(ModbusClient& modbusClient,
                 std::map<std::string, std::unique_ptr<DevicesConfigurationTemplate>>& configurationTemplates,
                 std::map<std::string, std::vector<int>>& deviceAddressesByTemplate,
                 std::map<int, std::unique_ptr<Device>>& devicesBySlaveAddress,
                 std::chrono::milliseconds registerReadPeriod);

    virtual ~ModbusBridge();

    void setOnSensorChange(
      const std::function<void(const std::string&, const std::string&, const std::string&)>& onSensorChange);

    void setOnActuatorStatusChange(
      const std::function<void(const std::string&, const std::string&, const std::string&)>& onActuatorStatusChange);

    void setOnAlarmChange(const std::function<void(const std::string&, const std::string&, bool)>& onAlarmChange);

    void setOnConfigurationChange(const std::function<void(const std::string&, void*)>& onConfigurationChange);

    void setOnDeviceStatusChange(
      const std::function<void(const std::string&, wolkabout::DeviceStatus::Status)>& onDeviceStatusChange);

    void start();
    void stop();

protected:
    void handleActuation(const std::string& deviceKey, const std::string& reference, const std::string& value) override;

    void handleConfiguration(const std::string& deviceKey,
                             const std::vector<ConfigurationItem>& configuration) override;

    ActuatorStatus getActuatorStatus(const std::string& deviceKey, const std::string& reference) override;

    std::vector<ConfigurationItem> getConfiguration(const std::string& deviceKey) override;

    DeviceStatus::Status getDeviceStatus(const std::string& deviceKey) override;

private:
    ActuatorStatus getActuatorStatusFromHoldingRegister(const ModbusRegisterMapping& modbusRegisterMapping,
                                                        ModbusRegisterWatcher& modbusRegisterWatcher);

    ActuatorStatus getActuatorStatusFromCoil(const ModbusRegisterMapping& modbusRegisterMapping,
                                             ModbusRegisterWatcher& modbusRegisterWatcher);

    ConfigurationItem getConfigurationStatusFromCoil(const ModbusRegisterMapping& modbusRegisterMapping,
                                                     ModbusRegisterWatcher& modbusRegisterWatcher);

    ConfigurationItem getConfigurationStatusFromHoldingRegister(const ModbusRegisterMapping& modbusRegisterMapping,
                                                                ModbusRegisterWatcher& modbusRegisterWatcher);

    void handleActuationForHoldingRegister(const ModbusRegisterMapping& modbusRegisterMapping,
                                           ModbusRegisterWatcher& modbusRegisterWatcher, const std::string& value);

    void handleActuationForCoil(const ModbusRegisterMapping& modbusRegisterMapping,
                                ModbusRegisterWatcher& modbusRegisterWatcher, const std::string& value);

    void handleConfigurationForHoldingRegister(const ModbusRegisterMapping& modbusRegisterMapping,
                                               ModbusRegisterWatcher& modbusRegisterWatcher, const std::string& value);

    void handleConfigurationForHoldingRegister(const ModbusRegisterMapping& modbusRegisterMapping,
                                               ModbusRegisterWatcher& modbusRegisterWatcher,
                                               std::vector<std::string> value);

    void handleConfigurationForCoil(const ModbusRegisterMapping& modbusRegisterMapping,
                                    ModbusRegisterWatcher& modbusRegisterWatcher, const std::string& value);

    void run();

    void readAndReportModbusRegistersValues();

    // The client
    ModbusClient& m_modbusClient;

    // Reconnect logic
    int m_timeoutIterator;
    const std::vector<int> m_timeoutDurations = {1, 5, 10, 15, 30, 60, 300, 600, 1800, 3600};
    bool m_shouldReconnect;

    // TODO figure out how to store all the devices, their ModbusRegisterGroups and their full necessary data.
    std::map<int, std::unique_ptr<Device>>& m_devices;
    std::map<int, std::vector<ModbusRegisterGroup>> m_registerGroups;
    std::map<std::string, ModbusRegisterMapping> m_referenceToModbusRegisterMapping;
    std::map<std::string, ModbusRegisterMapping> m_referenceToConfigurationModbusRegisterMapping;
    std::map<std::string, ModbusRegisterWatcher> m_referenceToModbusRegisterWatcherMapping;

    // Running logic
    std::chrono::milliseconds m_registerReadPeriod;
    std::atomic_bool m_readerShouldRun;
    std::unique_ptr<std::thread> m_reader;

    // All the callbacks from the modbusBridge to explicitly target Wolk instance and notify of data
    // TODO make all onData events explicit! RADICAL CHANGE OF THESE
    // This may involve adding methods to WolkGatewayModule-SDK-Cpp, like it was done before for alarms&configurations
    std::function<void(const std::string& deviceKey, const std::string& reference, const std::string& value)>
      m_onSensorChange;
    std::function<void(const std::string& deviceKey, const std::string& reference, const std::string& value)>
      m_onActuatorStatusChange;
    std::function<void(const std::string& deviceKey, const std::string& reference, bool active)> m_onAlarmChange;
    // TODO figure out what data needs to be here in the configuration
    std::function<void(const std::string& deviceKey, void* data)> m_onConfigurationChange;
    std::function<void(const std::string& deviceKey, wolkabout::DeviceStatus::Status status)> m_onDeviceStatusChange;
};
}    // namespace wolkabout

#endif
