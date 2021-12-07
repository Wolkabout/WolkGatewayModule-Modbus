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
#include "api/PlatformStatusListener.h"
#include "core/model/ConfigurationItem.h"
#include "model/Device.h"
#include "model/DevicesConfigurationTemplate.h"
#include "module/SafeModeValuePersistence.h"
#include "more_modbus/ModbusReader.h"

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

/**
 * @brief Class connecting two external interfaces, Modbus and the platform.
 * @details This class contains the connection between two external sides.
 *          Both to the MoreModbus library, with the Modbus devices,
 *          and to the platform with callbacks that trigger the Wolk object.
 */
class ModbusBridge : public ActuationHandlerPerDevice,
                     public ActuatorStatusProviderPerDevice,
                     public ConfigurationHandlerPerDevice,
                     public ConfigurationProviderPerDevice,
                     public DeviceStatusProvider,
                     public PlatformStatusListener
{
public:
    /**
     * @brief Default constructor for the class, initializes all the necessary data for workings.
     * @details Initializes the ModbusDevices according to the passed user configuration,
     *          first by creating the templates for such devices, where a whole device with slaveAddress
     *          -1 is created, and then copied over foreach device of such template.
     *          Value/status change listeners are also setup in here, next to all the maps of data/pointers
     *          that are necessary, such as mapping maps, status maps.
     * @param modbusClient setup client that will be used for the modbus reader.
     * @param configurationTemplates all configuration templates,
     * @param deviceAddressesByTemplate
     * @param devices
     * @param registerReadPeriod
     */
    ModbusBridge(ModbusClient& modbusClient,
                 const std::map<std::string, std::unique_ptr<DevicesConfigurationTemplate>>& configurationTemplates,
                 const std::map<std::string, std::vector<int>>& deviceAddressesByTemplate,
                 const std::map<int, std::unique_ptr<Device>>& devices, std::chrono::milliseconds registerReadPeriod);

    ~ModbusBridge() override;

    /**
     * @brief Get the running status of the Modbus reader.
     * @return
     */
    bool isRunning() const;

    void setOnSensorChange(const std::function<void(const std::string& deviceKey, const std::string& reference,
                                                    const std::string& value)>& onSensorChange);

    void setOnActuatorStatusChange(const std::function<void(const std::string& deviceKey, const std::string& reference,
                                                            const std::string& value)>& onActuatorStatusChange);

    void setOnAlarmChange(const std::function<void(const std::string& deviceKey, const std::string& reference,
                                                   bool active)>& onAlarmChange);

    void setOnConfigurationChange(const std::function<void(const std::string& deviceKey)>& onConfigurationChange);

    void setOnDeviceStatusChange(
      const std::function<void(const std::string& deviceKey, wolkabout::DeviceStatus::Status status)>&
        onDeviceStatusChange);

    /**
     * @brief Start the modbus reader logic.
     */
    void start();

    /**
     * @brief Stop the modbus reader logic.
     */
    void stop();

    void platformStatus(ConnectivityStatus status) override;

protected:
    /**
     * @brief Overridden method from ActuatorStatusProviderDevice. Return the Actuator status
     *        for specific device key and reference.
     * @param deviceKey called device by reference from the platform.
     * @param reference called actuator by reference from the platform.
     * @return
     */
    ActuatorStatus getActuatorStatus(const std::string& deviceKey, const std::string& reference) override;

    /**
     * @brief Overridden method for ActuationHandlerPerDevice. Handles actuation for specific device
     *        and reference.
     * @param deviceKey called device by reference from the platform.
     * @param reference called actuator by reference from the platform.
     * @param value new value for the actuator, received by the platform.
     */
    void handleActuation(const std::string& deviceKey, const std::string& reference, const std::string& value) override;

    /**
     * @brief Overridden method for ConfigurationProviderPerDevice. Return the Configuration status
     *        for specific device, referenced by key.
     * @param deviceKey called device by reference from the platform.
     * @return data from all the configuration mappings of device.
     */
    std::vector<ConfigurationItem> getConfiguration(const std::string& deviceKey) override;

    /**
     * @brief Overridden method for ConfigurationHandlerPerDevice. Handles configuration change for specific
     *         device and references.
     * @param deviceKey called device by reference from the platform.
     * @param configuration is a list of ConfigurationItems that target one or more Configuration references of device.
     */
    void handleConfiguration(const std::string& deviceKey,
                             const std::vector<ConfigurationItem>& configuration) override;

    /**
     * @brief Overridden method for DeviceStatusProvider. Returns device status for specific device.
     * @param deviceKey called device by reference from the platform.
     * @return device status for specific device.
     */
    DeviceStatus::Status getDeviceStatus(const std::string& deviceKey) override;

private:
    static void writeToMapping(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value);
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

    /**
     * @brief Helper method for getActuatorStatus, for returning status for mappings
     *        where register type is HOLDING_REGISTER.
     */
    static ActuatorStatus getActuatorStatusFromHoldingRegister(const std::shared_ptr<RegisterMapping>& mapping);

    /**
     * @brief Helper method for getActuatorStatus, for returning status for mappings
     *        where register type is COIL.
     */
    static ActuatorStatus getActuatorStatusFromCoil(const std::shared_ptr<RegisterMapping>& mapping);

    /**
     * @brief Helper method for getConfiguration, for returning status for mappings
     *        where register type is HOLDING_REGISTER.
     */
    static std::string getConfigurationStatusFromHoldingRegister(const std::shared_ptr<RegisterMapping>& mapping);

    /**
     * @brief Helper method for getConfiguration, for returning status for mappings
     *        where register type is COIL.
     */
    static std::string getConfigurationStatusFromCoil(const std::shared_ptr<RegisterMapping>& mapping);

    /**
     * @brief Helper method for handleActuation, handling value for mappings
     *        where register type is HOLDING_REGISTER.
     */
    static void handleActuationForHoldingRegister(const std::shared_ptr<RegisterMapping>& mapping,
                                                  const std::string& value);

    /**
     * @brief Helper method for handleActuation, handling value for mappings
     *        where register type is COIL.
     */
    static void handleActuationForCoil(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value);

    /**
     * @brief Helper method for handleConfiguration, handling value for mappings
     *        where register type is HOLDING_REGISTER.
     */
    static void handleConfigurationForHoldingRegister(const std::shared_ptr<RegisterMapping>& mapping,
                                                      const std::string& value);

    /**
     * @brief Helper method for handleConfiguration, handling value for mappings
     *        where register type is COIL.
     */
    static void handleConfigurationForCoil(const std::shared_ptr<RegisterMapping>& mapping, const std::string& value);

    // Methods to help with data query
    int getSlaveAddress(const std::string& deviceKey);

    static const char SEPARATOR;

    // The client
    ModbusClient& m_modbusClient;

    // The reader
    std::shared_ptr<ModbusReader> m_modbusReader;
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
    std::map<std::string, std::string> m_safeModeMappingByReference;
    std::map<std::string, ModuleMapping::MappingType> m_registerMappingTypeByReference;
    // Configurations grouped per device. Necessary for getConfiguration.
    std::map<std::string, std::vector<std::string>> m_configurationMappingByDeviceKey;
    std::map<std::string, std::map<std::string, std::shared_ptr<RegisterMapping>>>
      m_configurationMappingByDeviceKeyAndRef;

    // Store connectivity status
    ConnectivityStatus m_connectivityStatus;

    // Here we store the persistence
    SafeModeValuePersistence m_safeModePersistence;

    // All the callbacks from the modbusBridge to explicitly target Wolk instance and notify of data
    std::function<void(const std::string& deviceKey, const std::string& reference, const std::string& value)>
      m_onSensorChange;
    std::function<void(const std::string& deviceKey, const std::string& reference, const std::string& value)>
      m_onActuatorStatusChange;
    std::function<void(const std::string& deviceKey, const std::string& reference, bool active)> m_onAlarmChange;
    std::function<void(const std::string& deviceKey)> m_onConfigurationChange;
    std::function<void(const std::string& deviceKey, wolkabout::DeviceStatus::Status status)> m_onDeviceStatusChange;
};
}    // namespace wolkabout

#endif
