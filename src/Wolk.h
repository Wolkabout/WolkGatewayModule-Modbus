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

#ifndef WOLK_H
#define WOLK_H

#include "ActuationHandlerPerDevice.h"
#include "ActuatorStatusProviderPerDevice.h"
#include "ConfigurationHandlerPerDevice.h"
#include "ConfigurationProviderPerDevice.h"
#include "InboundGatewayMessageHandler.h"
#include "WolkBuilder.h"
#include "model/ActuatorStatus.h"
#include "model/Device.h"
#include "model/DeviceRegistrationResponse.h"
#include "model/DeviceStatus.h"
#include "protocol/DataProtocol.h"
#include "protocol/RegistrationProtocol.h"
#include "protocol/StatusProtocol.h"
#include "utilities/CommandBuffer.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class ConnectivityService;
class DataService;
class DeviceStatusService;
class DeviceRegistrationService;
class InboundMessageHandler;
class FirmwareUpdateService;
class FileDownloadService;

class Wolk
{
    friend class WolkBuilder;

public:
    ~Wolk();

    /**
     * @brief Initiates wolkabout::WolkBuilder that configures device to connect
     * to WolkAbout IoT Cloud
     * @param device wolkabout::Device
     * @return wolkabout::WolkBuilder instance
     */
    static WolkBuilder newBuilder();

    /**
     * @brief Publishes sensor reading to WolkAbout IoT Cloud<br>
     *        This method is thread safe, and can be called from multiple thread
     * simultaneously
     * @param deviceKey key of the device that holds the sensor
     * @param reference Sensor reference
     * @param value Sensor value<br>
     *              Supported types:<br>
     *               - bool<br>
     *               - float<br>
     *               - double<br>
     *               - signed int<br>
     *               - signed long int<br>
     *               - signed long long int<br>
     *               - unsigned int<br>
     *               - unsigned long int<br>
     *               - unsigned long long int<br>
     *               - string<br>
     *               - char*<br>
     *               - const char*<br>
     * @param rtc Reading POSIX time - Number of seconds since 01/01/1970<br>
     *            If omitted current POSIX time is adopted
     */
    template <typename T>
    void addSensorReading(const std::string& deviceKey, const std::string& reference, T value,
                          unsigned long long int rtc = 0);

    /**
     * @brief Publishes multi-value sensor reading to WolkAbout IoT Cloud<br>
     *        This method is thread safe, and can be called from multiple thread simultaneously
     * @param reference Sensor reference
     * @param values Multi-value sensor values<br>
     *              Supported types:<br>
     *               - bool<br>
     *               - float<br>
     *               - double<br>
     *               - signed int<br>
     *               - signed long int<br>
     *               - signed long long int<br>
     *               - unsigned int<br>
     *               - unsigned long int<br>
     *               - unsigned long long int<br>
     *               - string<br>
     *               - char*<br>
     *               - const char*<br>
     * @param rtc Reading POSIX time - Number of seconds since 01/01/1970<br>
     *            If omitted current POSIX time is adopted
     */
    template <typename T>
    void addSensorReading(const std::string& deviceKey, const std::string& reference, std::initializer_list<T> values,
                          unsigned long long int rtc = 0);

    /**
     * @brief Publishes multi-value sensor reading to WolkAbout IoT Cloud<br>
     *        This method is thread safe, and can be called from multiple thread simultaneously
     * @param reference Sensor reference
     * @param values Multi-value sensor values<br>
     *              Supported types:<br>
     *               - bool<br>
     *               - float<br>
     *               - double<br>
     *               - signed int<br>
     *               - signed long int<br>
     *               - signed long long int<br>
     *               - unsigned int<br>
     *               - unsigned long int<br>
     *               - unsigned long long int<br>
     *               - string<br>
     *               - char*<br>
     *               - const char*<br>
     * @param rtc Reading POSIX time - Number of seconds since 01/01/1970<br>
     *            If omitted current POSIX time is adopted
     */
    template <typename T>
    void addSensorReading(const std::string& deviceKey, const std::string& reference, const std::vector<T> values,
                          unsigned long long int rtc = 0);

    /**
     * @brief Publishes alarm to WolkAbout IoT Cloud<br>
     *        This method is thread safe, and can be called from multiple thread
     * simultaneously
     * @param deviceKey key of the device that holds the alarm
     * @param reference Alarm reference
     * @param active Is alarm active or not
     * @param rtc POSIX time at which event occurred - Number of seconds since
     * 01/01/1970<br> If omitted current POSIX time is adopted
     */
    void addAlarm(const std::string& deviceKey, const std::string& reference, bool active,
                  unsigned long long int rtc = 0);

    /**
     * @brief Invokes ActuatorStatusProvider callback to obtain actuator
     * status<br> This method is thread safe, and can be called from multiple
     * thread simultaneously
     * @param deviceKey key of the device that holds the actuator
     * @param Actuator reference
     */
    void publishActuatorStatus(const std::string& deviceKey, const std::string& reference);

    /**
     * @brief addDeviceStatus
     * @param status
     */
    void addDeviceStatus(const std::string& deviceKey, DeviceStatus status);

    /**
     * @brief Invokes ConfigurationProvider to obtain device configuration, and the publishes it.<br>
     *        This method is thread safe, and can be called from multiple thread simultaneously
     * * @param deviceKey key of the device that holds the configuration
     */
    void publishConfiguration(const std::string& deviceKey);

    /**
     * @brief connect Establishes connection with WolkAbout IoT platform
     */
    void connect();

    /**
     * @brief disconnect Disconnects from WolkAbout IoT platform
     */
    void disconnect();

    /**
     * @brief publish Publishes data
     */
    void publish();

    /**
     * @brief publish Publishes data
     */
    void publish(const std::string& deviceKey);

    /**
     * @brief addDevice
     * @param device
     */
    void addDevice(const Device& device);

    /**
     * @brief removeDevice
     * @param deviceKey
     */
    void removeDevice(const std::string& deviceKey);

private:
    class ConnectivityFacade;

    Wolk();

    void addToCommandBuffer(std::function<void()> command);

    static unsigned long long int currentRtc();

    void handleActuatorSetCommand(const std::string& key, const std::string& reference, const std::string& value);
    void handleActuatorGetCommand(const std::string& key, const std::string& reference);
    void handleDeviceStatusRequest(const std::string& key);
    void handleConfigurationSetCommand(const std::string& key, const std::vector<ConfigurationItem>& configuration);
    void handleConfigurationGetCommand(const std::string& key);

    void registerDevices();
    void registerDevice(const Device& device);

    std::vector<std::string> getDeviceKeys();
    bool deviceExists(const std::string& deviceKey);
    bool sensorDefinedForDevice(const std::string& deviceKey, const std::string& reference);
    std::string getSensorDelimiter(const std::string& deviceKey, const std::string& reference);
    std::map<std::string, std::string> getConfigurationDelimiters(const std::string& deviceKey);
    bool alarmDefinedForDevice(const std::string& deviceKey, const std::string& reference);
    bool actuatorDefinedForDevice(const std::string& deviceKey, const std::string& reference);
    bool configurationItemDefinedForDevice(const std::string& deviceKey, const std::string& reference);

    void handleRegistrationResponse(const std::string& deviceKey, DeviceRegistrationResponse::Result result);

    std::unique_ptr<ConnectivityService> m_connectivityService;

    std::unique_ptr<DataProtocol> m_dataProtocol;
    std::unique_ptr<StatusProtocol> m_statusProtocol;
    std::unique_ptr<RegistrationProtocol> m_registrationProtocol;

    std::unique_ptr<Persistence> m_persistence;

    std::unique_ptr<InboundGatewayMessageHandler> m_inboundMessageHandler;

    std::shared_ptr<ConnectivityFacade> m_connectivityManager;

    std::function<void(const std::string&, const std::string&, const std::string&)> m_actuationHandlerLambda;
    std::shared_ptr<ActuationHandlerPerDevice> m_actuationHandler;

    std::function<ActuatorStatus(const std::string&, const std::string&)> m_actuatorStatusProviderLambda;
    std::shared_ptr<ActuatorStatusProviderPerDevice> m_actuatorStatusProvider;

    std::function<DeviceStatus(const std::string&)> m_deviceStatusProviderLambda;
    std::shared_ptr<DeviceStatusProvider> m_deviceStatusProvider;

    std::function<void(const std::string&, const std::vector<ConfigurationItem>& configuration)>
      m_configurationHandlerLambda;
    std::shared_ptr<ConfigurationHandlerPerDevice> m_configurationHandler;

    std::function<std::vector<ConfigurationItem>(const std::string&)> m_configurationProviderLambda;
    std::shared_ptr<ConfigurationProviderPerDevice> m_configurationProvider;

    std::shared_ptr<DataService> m_dataService;
    std::shared_ptr<DeviceStatusService> m_deviceStatusService;
    std::shared_ptr<DeviceRegistrationService> m_deviceRegistrationService;

    std::map<std::string, Device> m_devices;

    std::atomic_bool m_connected;

    std::unique_ptr<CommandBuffer> m_commandBuffer;

    class ConnectivityFacade : public ConnectivityServiceListener
    {
    public:
        ConnectivityFacade(InboundMessageHandler& handler, std::function<void()> connectionLostHandler);

        void messageReceived(const std::string& channel, const std::string& message) override;
        void connectionLost() override;
        std::vector<std::string> getChannels() const override;

    private:
        InboundMessageHandler& m_messageHandler;
        std::function<void()> m_connectionLostHandler;
    };
};
}    // namespace wolkabout

#endif
