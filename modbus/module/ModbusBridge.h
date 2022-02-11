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

#include "core/model/Device.h"
#include "core/utilities/Logger.h"
#include "modbus/model/DeviceTemplate.h"
#include "modbus/module/persistence/KeyValuePersistence.h"
#include "more_modbus/ModbusReader.h"
#include "wolk/api/FeedUpdateHandler.h"
#include "wolk/api/ParameterHandler.h"
#include "wolk/api/PlatformStatusListener.h"

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
namespace more_modbus
{
class ModbusClient;
}

namespace modbus
{
class ModuleMapping;

/**
 * @brief Class connecting two external interfaces, Modbus and the platform.
 * @details This class contains the connection between two external sides.
 *          Both to the MoreModbus library, with the Modbus devices,
 *          and to the platform with callbacks that trigger the Wolk object.
 */
class ModbusBridge : public connect::FeedUpdateHandler,
                     public connect::ParameterHandler,
                     public connect::PlatformStatusListener
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
    ModbusBridge(std::shared_ptr<more_modbus::ModbusClient> modbusClient, std::chrono::milliseconds registerReadPeriod,
                 std::unique_ptr<KeyValuePersistence> defaultValuePersistence,
                 std::unique_ptr<KeyValuePersistence> repeatValuePersistence,
                 std::unique_ptr<KeyValuePersistence> safeModePersistence);

    /**
     * Default overridden destructor.
     * Will stop the modbus reader and disconnect the modbus client.
     */
    ~ModbusBridge() override;

    /**
     * This is the first method that anyone using this bridge should invoke immediately.
     * This is the method that creates the necessary entities for the bridge to do it's job.
     *
     * @param templates This is the map containing the templates that are imported from the
     * DevicesConfiguration.
     * @param deviceAddressesByTemplate This is the map containing the list of devices that belong to each template.
     * @param devices This is the map of generated devices by the device generator.
     */
    virtual void initialize(const std::map<std::string, std::unique_ptr<DeviceTemplate>>& templates,
                            const std::map<std::string, std::vector<std::uint16_t>>& deviceAddressesByTemplate,
                            const std::map<std::uint16_t, std::unique_ptr<Device>>& devices);

    /**
     * @brief Get the running status of the Modbus reader.
     * @return
     */
    bool isRunning() const;

    /**
     * @brief Setter for the callback which will be invoked once a feed value has been updated.
     * @param feedValueCallback The callback that will be invoked once a feed value has been updated.
     */
    void setFeedValueCallback(const std::function<void(const std::string&, const Reading&)>& feedValueCallback);

    /**
     * @brief Setter for the callback which will be invoked once an attribute value has been updated.
     * @param attributeCallback The callback that will be invoked once an attribute value has been updated.
     */
    void setAttributeCallback(const std::function<void(const std::string&, const Attribute&)>& attributeCallback);

    /**
     * @brief Start the modbus reader logic.
     */
    void start();

    /**
     * @brief Stop the modbus reader logic.
     */
    void stop();

    /**
     * This method is overridden from the `connect::PlatformStatusListener` interface.
     * This method is invoked once a new ConnectivityStatus value has been received.
     *
     * @param status The new ConnectivityStatus value, received from the Gateway.
     */
    void platformStatus(ConnectivityStatus status) override;

    /**
     * This is the overridden method from the `connect::FeedUpdateHandler` interface.
     * This is the method that is invoked once a new Feed value has been sent out by the platform.
     *
     * @param deviceKey The device for which the new values have been sent.
     * @param readings The reading values that have been sent out for the device.
     */
    void handleUpdate(const std::string& deviceKey,
                      const std::map<std::uint64_t, std::vector<Reading>>& readings) override;

    /**
     * This is the overridden method from the `connect::ParameterHandler` interface.
     * This is the method that is invoked once a new Parameter value has been sent out by the platform.
     *
     * @param deviceKey The device for which the new values have been sent.
     * @param parameters The parameter values that have been sent out for the device.
     */
    void handleUpdate(const std::string& deviceKey, const std::vector<Parameter>& parameters) override;

private:
    /**
     * This is a part of the initialize that will set up the device callbacks.
     *
     * @param devices The devices for which the callbacks need to be set up.
     */
    void initializeSetUpDeviceCallback(const std::vector<std::shared_ptr<more_modbus::ModbusDevice>>& device);

    /**
     * This is a helper method that is used to write in a map of values into the mappings.
     *
     * @param mapOfValues A map containing references to mappings and values for them.
     */
    void writeAMapOfValues(const std::map<std::string, std::string>& mapOfValues);

    /**
     * This is a helper method that is used to go up a chain of weak pointers to attempt to notify a device of a mapping
     * value change. This is used for when a boolean value has changed.
     *
     * @param mapping The mapping that changed its value.
     */
    static void triggerGroupValueChangeBool(const std::shared_ptr<more_modbus::RegisterMapping>& mapping);

    /**
     * This is a helper method that is used to go up a chain of weak pointers to attempt to notify a device of a mapping
     * value change. This is used for when a value in bytes has changed.
     *
     * @param mapping The mapping that changed its value.
     */
    static void triggerGroupValueChangeBytes(const std::shared_ptr<more_modbus::RegisterMapping>& mapping);

    /**
     * This is a helper method that is used to initiate a value write into a mapping.
     * This call routes the call to a downcast version of the mapping, where the specific `writeValue` method of the
     * mapping type will be called.
     *
     * @param mapping The mapping pointer of the mapping that needs to change.
     * @param value The new value for the mapping.
     */
    void writeToMapping(const std::shared_ptr<more_modbus::RegisterMapping>& mapping, const std::string& value);

    /**
     * This is a helper method that will downcast the pointer and invoke the right method for the specific mapping.
     *
     * @tparam RegisterMappingType The specific type for the mapping, a type that derives from
     * `more_modbus::RegisterMapping` and contains a method `writeValue`.
     * @tparam Value The type of the value that is passed to the `writeValue` call.
     * @param mapping The mapping pointer of the mapping that needs to change.
     * @param value The new value for the mapping already converted into the right type.
     */
    template <class RegisterMappingType, class Value>
    void writeToMappingSpecific(const std::shared_ptr<more_modbus::RegisterMapping>& mapping, const Value& value);

    /**
     * This is a helper method that will form a reading for the mapping based on its type. It will use the new data
     * that has been read from the mapping.
     *
     * @param mapping The mapping from which the value needs to be obtained.
     * @param bytes The bytes that form the data.
     * @return The reading that has been formed for the new data.
     */
    Reading formReadingForMappingValue(const std::shared_ptr<more_modbus::RegisterMapping>& mapping,
                                       const std::vector<std::uint16_t>& bytes);

    /**
     * This is a helper method that will form an attribute for the mapping based on its type. It will use the new data
     * that has been read from the mapping.
     *
     * @param mapping The mapping from which the value needs to be obtained.
     * @param bytes The bytes that form the data.
     * @return The attribute that has been formed for the new data.
     */
    Attribute formAttributeForMappingValue(const std::shared_ptr<more_modbus::RegisterMapping>& mapping,
                                           const std::vector<std::uint16_t>& bytes);

    /**
     * This is a helper method for the `handleUpdate` method that returns whether a Reading is meant to change a
     * DefaultValue of a different feed.
     *
     * @param reading The reading that should be checked.
     * @return Whether the reading is meant to change a DefaultValue of a different feed.
     */
    static bool isDefaultValueReading(const Reading& reading);

    /**
     * This is a helper method for the `handleUpdate` method that handles a Reading meant to change a DefaultValue of a
     * different feed.
     *
     * @param reading The reading that contains a new value for a DefaultValue of a feed.
     */
    void handleDefaultValueReading(const std::string& deviceKey, const Reading& reading);

    /**
     * This is a helper method for the `handleUpdate` method that returns whether a Reading is meant to change a
     * RepeatWrite of a different feed.
     *
     * @param reading The reading that should be checked.
     * @return Whether the reading is meant to change a RepeatWrite of a different feed.
     */
    static bool isRepeatWriteReading(const Reading& reading);

    /**
     * This is a helper method for the `handleUpdate` method that handles a Reading meant to change a RepeatWrite of a
     * different feed.
     *
     * @param reading The reading that contains a new value for a RepeatWrite of a feed.
     */
    void handleRepeatWriteReading(const std::string& deviceKey, const Reading& reading);

    /**
     * This is a helper method for the `handleUpdate` method that returns whether a Reading is meant to change a
     * SafeModeValue of a different feed.
     *
     * @param reading The reading that should be checked.
     * @return Whether the reading is meant to change a SafeModeValue of a different feed.
     */
    static bool isSafeModeValueReading(const Reading& reading);

    /**
     * This is a helper method for the `handleUpdate` method that handles a Reading meant to change a SafeModeValue of a
     * different feed.
     *
     * @param reading The reading that contains a new value for a SafeModeValue of a feed.
     */
    void handleSafeModeValueReading(const std::string& deviceKey, const Reading& reading);

    // Methods to help with data query
    int getSlaveAddress(const std::string& deviceKey);

    // Separator used in maps for device key/reference combinations
    static const char SEPARATOR;
    const std::string TAG = "[ModbusBridge] -> ";

    // The client
    std::shared_ptr<more_modbus::ModbusClient> m_modbusClient;

    // The reader
    std::shared_ptr<more_modbus::ModbusReader> m_modbusReader;
    std::chrono::milliseconds m_registerReadPeriod;

    // Used to fast decode deviceKey by slaveAddress.
    std::map<int, std::string> m_deviceKeyBySlaveAddress;
    // Device status
    // Watcher for all the mappings. This is the shortcut for handle and get queries to get to the mapping they need.
    std::map<std::string, std::shared_ptr<more_modbus::RegisterMapping>> m_registerMappingByReference;
    std::map<std::string, std::string> m_defaultValueMappingByReference;
    std::map<std::string, std::chrono::milliseconds> m_repeatedWriteMappingByReference;
    std::map<std::string, std::string> m_safeModeMappingByReference;
    std::map<std::string, MappingType> m_registerMappingTypeByReference;

    // Store connectivity status
    ConnectivityStatus m_connectivityStatus;

    // Here we store the persistence
    std::unique_ptr<KeyValuePersistence> m_defaultValuePersistence;
    std::unique_ptr<KeyValuePersistence> m_repeatValuePersistence;
    std::unique_ptr<KeyValuePersistence> m_safeModePersistence;

    // Callbacks that will be invoked with new values once they appear
    std::function<void(const std::string& deviceKey, const Reading& reading)> m_feedValueCallback;
    std::function<void(const std::string& deviceKey, const Attribute& attribute)> m_attributeCallback;
};

template <class RegisterMappingType, class Value>
void ModbusBridge::writeToMappingSpecific(const std::shared_ptr<more_modbus::RegisterMapping>& mapping,
                                          const Value& value)
{
    // Downcast the mapping pointer
    const auto castMapping = std::dynamic_pointer_cast<RegisterMappingType>(mapping);
    if (castMapping == nullptr)
    {
        LOG(WARN) << TAG << "Failed to downcast the RegisterMapping to '" << typeid(RegisterMappingType).name() << "'.";
        return;
    }

    // Write in the value
    try
    {
        castMapping->writeValue(value);
    }
    catch (const std::exception& exception)
    {
        LOG(WARN) << TAG << "Failed to write value to '" << typeid(RegisterMappingType).name() << "' -> '"
                  << exception.what() << "'.";
    }
}
}    // namespace modbus
}    // namespace wolkabout

#endif
