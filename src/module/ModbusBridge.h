/*
 * Copyright 2020 WolkAbout Technology s.r.o.
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
#include "ModbusReader.h"
#include "model/ConfigurationItem.h"
#include "model/Device.h"
#include "model/DevicesConfigurationTemplate.h"

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
class ModuleMapping;
class ModbusClient;

class ModbusBridge : public ActuationHandlerPerDevice,
                     public ActuatorStatusProviderPerDevice,
                     public ConfigurationHandlerPerDevice,
                     public ConfigurationProviderPerDevice,
                     public DeviceStatusProvider
{
public:
    ModbusBridge(ModbusClient& modbusClient,
                 const std::map<std::string, std::unique_ptr<DevicesConfigurationTemplate>>& configurationTemplates,
                 const std::map<std::string, std::vector<int>>& deviceAddressesByTemplate,
                 const std::map<int, std::unique_ptr<Device>>& devices, std::chrono::milliseconds registerReadPeriod);

    ~ModbusBridge() override;

    bool isRunning() const;

    void setOnSensorChange(const std::function<void(const std::string& deviceKey, const std::string& reference,
                                                    const std::string& value)>& onSensorChange);

    void setOnActuatorStatusChange(const std::function<void(const std::string& deviceKey, const std::string& reference,
                                                            const std::string& value)>& onActuatorStatusChange);

    void setOnAlarmChange(const std::function<void(const std::string& deviceKey, const std::string& reference,
                                                   bool active)>& onAlarmChange);

    void setOnConfigurationChange(
      const std::function<void(const std::string& deviceKey, std::vector<ConfigurationItem>& data)>&
        onConfigurationChange);

    void setOnDeviceStatusChange(
      const std::function<void(const std::string& deviceKey, wolkabout::DeviceStatus::Status status)>&
        onDeviceStatusChange);

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
    static void writeToBoolMapping(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value);
    static void writeToUInt16Mapping(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value);
    static void writeToInt16Mapping(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value);
    static void writeToUInt32Mapping(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value);
    static void writeToInt32Mapping(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value);
    static void writeToFloatMapping(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value);
    static void writeToStringMapping(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value);

    static std::string readFromMapping(const std::shared_ptr<RegisterMapping>& mapping);
    static std::string readFromBoolMapping(const std::shared_ptr<RegisterMapping>& mapping);
    static std::string readFromUInt16Mapping(const std::shared_ptr<RegisterMapping>& mapping);
    static std::string readFromInt16Mapping(const std::shared_ptr<RegisterMapping>& mapping);
    static std::string readFromUInt32Mapping(const std::shared_ptr<RegisterMapping>& mapping);
    static std::string readFromInt32Mapping(const std::shared_ptr<RegisterMapping>& mapping);
    static std::string readFromFloatMapping(const std::shared_ptr<RegisterMapping>& mapping);
    static std::string readFromStringMapping(const std::shared_ptr<RegisterMapping>& mapping);

    // Helper methods for getActuatorStatus
    static ActuatorStatus getActuatorStatusFromHoldingRegister(const std::shared_ptr<RegisterMapping>& mapping);

    static ActuatorStatus getActuatorStatusFromCoil(const std::shared_ptr<RegisterMapping>& mapping);

    // Helper methods for getConfiguration
    static std::string getConfigurationStatusFromCoil(const std::shared_ptr<RegisterMapping>& mapping);

    static std::string getConfigurationStatusFromHoldingRegister(const std::shared_ptr<RegisterMapping>& mapping);

    // Helper methods for handleActuation
    static void handleActuationForHoldingRegister(const std::shared_ptr<RegisterMapping>& mapping,
                                                  const std::string& value);

    static void handleActuationForCoil(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value);

    // Helper methods for handleConfiguration
    static void handleConfigurationForHoldingRegister(const std::shared_ptr<RegisterMapping>& mapping,
                                                      const std::string& value);

    static void handleConfigurationForCoil(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value);

    // Methods to help with data query
    int getSlaveAddress(const std::string& deviceKey);

    static const char SEPARATOR;

    // The client
    ModbusClient& m_modbusClient;

    // The reader
    std::unique_ptr<ModbusReader> m_modbusReader;
    std::chrono::milliseconds m_registerReadPeriod;

    // True values
    static const std::vector<std::string> TRUE_VALUES;

    // Used to fast decode deviceKey by slaveAddress.
    std::map<int, std::string> m_deviceKeyBySlaveAddress;
    // Device status
    // Mostly to be used by getDeviceStatus to provide, and readAndReport to write if device is available
    std::map<int, DeviceStatus::Status> m_devicesStatusBySlaveAddress;
    // Watcher for all the mappings. This is the shortcut for handle and get queries to get to the mapping they need.
    std::map<std::string, std::shared_ptr<RegisterMapping>> m_registerMappingByReference;
    std::map<std::string, ModuleMapping::MappingType> m_registerMappingTypeByReference;
    // Configurations grouped per device. Necessary for getConfiguration.
    std::map<std::string, std::vector<std::string>> m_configurationMappingByDeviceKey;
    std::map<std::string, std::map<std::string, std::shared_ptr<RegisterMapping>>>
      m_configurationMappingByDeviceKeyAndRef;

    // All the callbacks from the modbusBridge to explicitly target Wolk instance and notify of data
    std::function<void(const std::string& deviceKey, const std::string& reference, const std::string& value)>
      m_onSensorChange;
    std::function<void(const std::string& deviceKey, const std::string& reference, const std::string& value)>
      m_onActuatorStatusChange;
    std::function<void(const std::string& deviceKey, const std::string& reference, bool active)> m_onAlarmChange;
    std::function<void(const std::string& deviceKey, std::vector<ConfigurationItem>& data)> m_onConfigurationChange;
    std::function<void(const std::string& deviceKey, wolkabout::DeviceStatus::Status status)> m_onDeviceStatusChange;
};
}    // namespace wolkabout

#endif
