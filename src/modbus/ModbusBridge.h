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
                 std::map<int, std::unique_ptr<Device>>& devices, std::chrono::milliseconds registerReadPeriod);

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
    // Interface method for ActuatorStatusProviderPerDevice
    // Used to handle request from platform to provide data about an actuator on device
    ActuatorStatus getActuatorStatus(const std::string& deviceKey, const std::string& reference) override;

    // Interface method for ActuationHandlerPerDevice
    // Used to receive data change about an actuator from platform on specific device
    void handleActuation(const std::string& deviceKey, const std::string& reference, const std::string& value) override;

    // Interface method for ConfigurationProviderPerDevice
    // Used to handle request from platform to provide data about a configuration on device
    std::vector<ConfigurationItem> getConfiguration(const std::string& deviceKey) override;

    // Interface method for ConfigurationHandlerPerDevice
    // Used to receive data change about a configuration from platform on specific device
    void handleConfiguration(const std::string& deviceKey,
                             const std::vector<ConfigurationItem>& configuration) override;

    // Interface method for DeviceStatusProvider
    DeviceStatus::Status getDeviceStatus(const std::string& deviceKey) override;

private:
    // Helper methods for getActuatorStatus
    // TODO Change the parameters
    ActuatorStatus getActuatorStatusFromHoldingRegister(const ModbusRegisterMapping& modbusRegisterMapping,
                                                        ModbusRegisterWatcher& modbusRegisterWatcher);

    ActuatorStatus getActuatorStatusFromCoil(const ModbusRegisterMapping& modbusRegisterMapping,
                                             ModbusRegisterWatcher& modbusRegisterWatcher);

    // Helper methods for getConfiguration
    // TODO Change the parameters
    ConfigurationItem getConfigurationStatusFromCoil(const ModbusRegisterMapping& modbusRegisterMapping,
                                                     ModbusRegisterWatcher& modbusRegisterWatcher);

    ConfigurationItem getConfigurationStatusFromHoldingRegister(const ModbusRegisterMapping& modbusRegisterMapping,
                                                                ModbusRegisterWatcher& modbusRegisterWatcher);

    // Helper methods for handleActuation
    // TODO Change the parameters
    void handleActuationForHoldingRegister(ModbusRegisterWatcher& watcher, const std::string& value);

    void handleActuationForCoil(ModbusRegisterWatcher& modbusRegisterWatcher, const std::string& value);

    // Helper methods for handleConfiguration
    // TODO Change the parameters
    void handleConfigurationForHoldingRegister(const ModbusRegisterMapping& modbusRegisterMapping,
                                               ModbusRegisterWatcher& modbusRegisterWatcher, const std::string& value);

    void handleConfigurationForHoldingRegisters(const ModbusRegisterMapping& modbusRegisterMapping,
                                                ModbusRegisterWatcher& modbusRegisterWatcher,
                                                std::vector<std::string> value);

    void handleConfigurationForCoil(const ModbusRegisterMapping& modbusRegisterMapping,
                                    ModbusRegisterWatcher& modbusRegisterWatcher, const std::string& value);

    // Running methods
    void run();

    // Helper methods for readAndReport
    void readHoldingRegisters(ModbusRegisterGroup& group);

    void readInputRegisters(ModbusRegisterGroup& group);

    void readCoils(ModbusRegisterGroup& group);

    void readDiscreteInputs(ModbusRegisterGroup& group);

    void readAndReportModbusRegistersValues();

    // Methods to help with data query
    int getSlaveAddress(const std::string& deviceKey);

    // The client
    ModbusClient& m_modbusClient;

    // Reconnect logic
    int m_timeoutIterator;
    const std::vector<int> m_timeoutDurations = {1, 5, 10, 15, 30, 60, 300, 600, 1800, 3600};
    bool m_shouldReconnect;

    // Used to fast decode deviceKey by slaveAddress.
    std::map<int, std::string> m_deviceKeyBySlaveAddress;
    // Access device by slaveAddress
    // Mostly to be used by the readAndReport method, for quickly being able read all groups of device
    std::map<int, std::vector<ModbusRegisterGroup>> m_registerGroupsBySlaveAddress;
    // Device status
    // Mostly to be used by getDeviceStatus to provide, and readAndReport to write if device is available
    std::map<int, DeviceStatus::Status> m_devicesStatusBySlaveAddress;
    // Watcher for all the mappings. This is the shortcut for handle and get queries to get to the mapping they need.
    std::map<std::string, ModbusRegisterWatcher> m_registerWatcherByReference;

    // Running logic data
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
