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

#ifndef WOLKBUILDER_H
#define WOLKBUILDER_H

#include "ActuationHandlerPerDevice.h"
#include "ActuatorStatusProviderPerDevice.h"
#include "ConfigurationHandlerPerDevice.h"
#include "ConfigurationProviderPerDevice.h"
#include "DeviceStatusProvider.h"
#include "connectivity/ConnectivityService.h"
#include "model/ActuatorStatus.h"
#include "model/Device.h"
#include "model/DeviceStatus.h"
#include "persistence/Persistence.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace wolkabout
{
class Wolk;
class DataProtocol;
class StatusProtocol;
class RegistrationProtocol;

class WolkBuilder final
{
public:
    ~WolkBuilder() = default;

    WolkBuilder(WolkBuilder&&) = default;

    /**
     * @brief WolkBuilder Initiates wolkabout::Wolk builder
     * @param device Device for which wolkabout::WolkBuilder is instantiated
     */
    WolkBuilder();

    /**
     * @brief Allows passing of URI to custom WolkAbout IoT platform instance
     * @param host Server URI
     * @return Reference to current wolkabout::WolkBuilder instance (Provides
     * fluent interface)
     */
    WolkBuilder& host(const std::string& host);

    /**
     * @brief Sets actuation handler
     * @param actuationHandler Callable that handles actuation requests
     * @return Reference to current wolkabout::WolkBuilder instance (Provides
     * fluent interface)
     */
    WolkBuilder& actuationHandler(const std::function<void(const std::string& deviceKey, const std::string& reference,
                                                           const std::string& value)>& actuationHandler);

    /**
     * @brief Sets actuation handler
     * @param actuationHandler Implementation that handles actuation requests
     * @return Reference to current wolkabout::WolkBuilder instance (Provides
     * fluent interface)
     */
    WolkBuilder& actuationHandler(std::shared_ptr<ActuationHandlerPerDevice> actuationHandler);

    /**
     * @brief Sets actuation status provider
     * @param actuatorStatusProvider Callable that provides ActuatorStatus by
     * reference of requested actuator
     * @return Reference to current wolkabout::WolkBuilder instance (Provides
     * fluent interface)
     */
    WolkBuilder& actuatorStatusProvider(
      const std::function<ActuatorStatus(const std::string& deviceKey, const std::string& reference)>&
        actuatorStatusProvider);

    /**
     * @brief Sets actuation status provider
     * @param actuatorStatusProvider Implementation that provides ActuatorStatus
     * by reference of requested actuator
     * @return Reference to current wolkabout::WolkBuilder instance (Provides
     * fluent interface)
     */
    WolkBuilder& actuatorStatusProvider(std::shared_ptr<ActuatorStatusProviderPerDevice> actuatorStatusProvider);

    /**
     * @brief Sets device configuration handler
     * @param configurationHandler Lambda that handles setting of configuration
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& configurationHandler(
      const std::function<void(const std::string& deviceKey, const std::vector<ConfigurationItem>& configuration)>&
        configurationHandler);

    /**
     * @brief Sets device configuration handler
     * @param configurationHandler Instance of wolkabout::ConfigurationHandler that handles setting of configuration
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& configurationHandler(std::shared_ptr<ConfigurationHandlerPerDevice> configurationHandler);

    /**
     * @brief Sets device configuration provider
     * @param configurationProvider Lambda that provides device configuration
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& configurationProvider(
      const std::function<std::vector<ConfigurationItem>(const std::string& deviceKey)>& configurationProvider);

    /**
     * @brief Sets device configuration provider
     * @param configurationProvider Instance of wolkabout::ConfigurationProvider that provides device configuration
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& configurationProvider(std::shared_ptr<ConfigurationProviderPerDevice> configurationProvider);

    /**
     * @brief Sets device status provider
     * @param deviceStatusProvider Callable that provides DeviceStatus by device
     * key
     * @return Reference to current wolkabout::WolkBuilder instance (Provides
     * fluent interface)
     */
    WolkBuilder& deviceStatusProvider(
      const std::function<DeviceStatus(const std::string& deviceKey)>& deviceStatusProvider);

    /**
     * @brief Sets device status provider
     * @param deviceStatusProvider Implementation that provides DeviceStatus
     * by device key
     * @return Reference to current wolkabout::WolkBuilder instance (Provides
     * fluent interface)
     */
    WolkBuilder& deviceStatusProvider(std::shared_ptr<DeviceStatusProvider> deviceStatusProvider);

    /**
     * @brief Sets underlying persistence mechanism to be used<br>
     *        Sample in-memory persistence is used as default
     * @param persistence std::shared_ptr to wolkabout::Persistence implementation
     * @return Reference to current wolkabout::WolkBuilder instance (Provides
     * fluent interface)
     */
    WolkBuilder& withPersistence(std::unique_ptr<Persistence> persistence);

    /**
     * @brief withDataProtocol Defines which data protocol to use
     * @param Protocol unique_ptr to wolkabout::DataProtocol implementation
     * @return Reference to current wolkabout::WolkBuilder instance (Provides
     * fluent interface)
     */
    WolkBuilder& withDataProtocol(std::unique_ptr<DataProtocol> protocol);

    /**
     * @brief Builds Wolk instance
     * @return Wolk instance as std::unique_ptr<Wolk>
     *
     * @throws std::logic_error if device key is not present in wolkabout::Device
     * @throws std::logic_error if actuator status provider is not set, and
     * wolkabout::Device has actuator references
     * @throws std::logic_error if actuation handler is not set, and
     * wolkabout::Device has actuator references
     */
    std::unique_ptr<Wolk> build();

    /**
     * @brief operator std::unique_ptr<Wolk> Conversion to wolkabout::wolk as
     * result returns std::unique_ptr to built wolkabout::Wolk instance
     */
    operator std::unique_ptr<Wolk>();

private:
    std::string m_host;

    std::function<void(const std::string&, const std::string&, const std::string&)> m_actuationHandlerLambda;
    std::shared_ptr<ActuationHandlerPerDevice> m_actuationHandler;

    std::function<ActuatorStatus(const std::string&, const std::string&)> m_actuatorStatusProviderLambda;
    std::shared_ptr<ActuatorStatusProviderPerDevice> m_actuatorStatusProvider;

    std::function<void(const std::string&, const std::vector<ConfigurationItem>& configuration)>
      m_configurationHandlerLambda;
    std::shared_ptr<ConfigurationHandlerPerDevice> m_configurationHandler;

    std::function<std::vector<ConfigurationItem>(const std::string&)> m_configurationProviderLambda;
    std::shared_ptr<ConfigurationProviderPerDevice> m_configurationProvider;

    std::function<DeviceStatus(const std::string&)> m_deviceStatusProviderLambda;
    std::shared_ptr<DeviceStatusProvider> m_deviceStatusProvider;

    std::unique_ptr<Persistence> m_persistence;

    std::unique_ptr<DataProtocol> m_dataProtocol;
    std::unique_ptr<StatusProtocol> m_statusProtocol;
    std::unique_ptr<RegistrationProtocol> m_registrationProtocol;

    static const constexpr char* MESSAGE_BUS_HOST = "tcp://localhost:1883";
};
}    // namespace wolkabout

#endif
