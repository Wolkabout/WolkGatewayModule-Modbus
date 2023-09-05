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

#include "modbus/module/ModbusBridge.h"

#include "MoreModbus/more_modbus/mappings/FloatMapping.h"
#include "MoreModbus/more_modbus/mappings/Int16Mapping.h"
#include "MoreModbus/more_modbus/mappings/Int32Mapping.h"
#include "MoreModbus/more_modbus/mappings/StringMapping.h"
#include "MoreModbus/more_modbus/mappings/UInt16Mapping.h"
#include "MoreModbus/more_modbus/mappings/UInt32Mapping.h"
#include "core/utilities/Logger.h"
#include "modbus/module/RegisterMappingFactory.h"
#include "more_modbus/ModbusDevice.h"
#include "more_modbus/mappings/BoolMapping.h"
#include "more_modbus/modbus/ModbusClient.h"
#include "more_modbus/utilities/DataParsers.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace wolkabout::modbus
{
const char ModbusBridge::SEPARATOR = '.';

ModbusBridge::ModbusBridge(std::shared_ptr<more_modbus::ModbusClient> modbusClient,
                           std::chrono::milliseconds registerReadPeriod,
                           std::unique_ptr<KeyValuePersistence> defaultValuePersistence,
                           std::unique_ptr<KeyValuePersistence> repeatValuePersistence,
                           std::unique_ptr<KeyValuePersistence> safeModePersistence)
: m_modbusClient(std::move(modbusClient))
, m_registerReadPeriod(registerReadPeriod)
, m_deviceKeyBySlaveAddress()
, m_registerMappingByReference()
, m_connectivityStatus(ConnectivityStatus::NONE)
, m_defaultValuePersistence(std::move(defaultValuePersistence))
, m_repeatValuePersistence(std::move(repeatValuePersistence))
, m_safeModePersistence(std::move(safeModePersistence))
{
}

ModbusBridge::~ModbusBridge()
{
    stop();
    if (m_modbusClient != nullptr)
        m_modbusClient->disconnect();
}

void ModbusBridge::initialize(const std::map<std::string, std::unique_ptr<DeviceTemplate>>& templates,
                              const std::map<std::string, std::vector<std::uint16_t>>& deviceAddressesByTemplate,
                              const std::map<std::uint16_t, std::unique_ptr<Device>>& devices)
{
    // Create the reader
    m_modbusReader = std::make_shared<more_modbus::ModbusReader>(*m_modbusClient, m_registerReadPeriod);

    // Create the list of devices we're going to add to the reader
    auto modbusDevices = std::vector<std::shared_ptr<more_modbus::ModbusDevice>>{};
    auto defaultValues = m_defaultValuePersistence->loadValues();
    auto repeatedValues = m_repeatValuePersistence->loadValues();
    auto safeModeValues = m_safeModePersistence->loadValues();

    // Go through the list of devices for every template, and copy the data for the template to each device.
    for (const auto& templateRegistered : deviceAddressesByTemplate)
    {
        // Create an initial list of mappings for the template.
        const auto& templateInfo = *(templates.at(templateRegistered.first));
        auto mappings = std::vector<std::shared_ptr<more_modbus::RegisterMapping>>{};
        auto defaultValueMappings = std::map<std::string, std::string>{};
        auto repeatValueMappings = std::map<std::string, std::chrono::milliseconds>{};
        auto safeMappings = std::map<std::string, std::string>{};
        auto mappingTypeByReference = std::map<std::string, MappingType>{};
        auto autoReadMappings = std::map<std::string, bool>{};

        // Go through the mappings of the template
        for (const auto& mapping : templateInfo.getMappings())
        {
            mappingTypeByReference.emplace(mapping.getReference(), mapping.getMappingType());
            autoReadMappings.emplace(mapping.getReference(), mapping.isAutoReadAfterWrite());

            // If any of the mappings are in the special categories
            if (!mapping.getDefaultValue().empty())
                defaultValueMappings.emplace(mapping.getReference(), mapping.getDefaultValue());
            if (mapping.getRepeat().count() > 0)
                repeatValueMappings.emplace(mapping.getReference(), mapping.getRepeat());
            if (mapping.hasSafeMode())
                safeMappings.emplace(mapping.getReference(), mapping.getSafeModeValue());
        }

        // Foreach device slaveAddress, copy over the mappings to create the device.
        for (const auto& slaveAddress : templateRegistered.second)
        {
            const auto key = devices.at(slaveAddress)->getKey();

            // Filter out the default, repeat and safe mode values for this device
            const auto defaultValuesForDevice = [&]()
            {
                auto map = std::map<std::string, std::string>{};
                std::copy_if(defaultValues.cbegin(), defaultValues.cend(), std::inserter(map, map.end()),
                             [&](const std::pair<std::string, std::string>& pair)
                             { return pair.first.find(key + SEPARATOR) != std::string::npos; });
                return map;
            }();
            const auto repeatValuesForDevice = [&]()
            {
                auto map = std::map<std::string, std::string>{};
                std::copy_if(repeatedValues.cbegin(), repeatedValues.cend(), std::inserter(map, map.end()),
                             [&](const std::pair<std::string, std::string>& pair)
                             { return pair.first.find(key + SEPARATOR) != std::string::npos; });
                return map;
            }();
            const auto safeModeValueForDevice = [&]()
            {
                auto map = std::map<std::string, std::string>{};
                std::copy_if(safeModeValues.cbegin(), safeModeValues.cend(), std::inserter(map, map.end()),
                             [&](const std::pair<std::string, std::string>& pair)
                             { return pair.first.find(key + SEPARATOR) != std::string::npos; });
                return map;
            }();

            const auto device = std::make_shared<more_modbus::ModbusDevice>(key, slaveAddress);

            mappings.clear();
            for (const auto& mapping : templateInfo.getMappings())
            {
                mappings.emplace_back(RegisterMappingFactory::fromJSONMapping(mapping));
            }

            device->createGroups(mappings);
            modbusDevices.emplace_back(device);

            m_deviceKeyBySlaveAddress.emplace(slaveAddress, key);

            // Register all the mappings into a map, keep configuration mappings special too.
            for (const auto& group : device->getGroups())
            {
                for (const auto& mapping : group->getMappings())
                {
                    m_registerMappingByReference.emplace(key + SEPARATOR + mapping.second->getReference(),
                                                         mapping.second);
                    m_registerMappingTypeByReference.emplace(key + SEPARATOR + mapping.second->getReference(),
                                                             mappingTypeByReference[mapping.second->getReference()]);

                    const auto defaultValueIt = defaultValueMappings.find(mapping.second->getReference());
                    if (defaultValueIt != defaultValueMappings.cend())
                    {
                        auto defaultValue = defaultValueIt->second;
                        const auto it = defaultValuesForDevice.find(key + SEPARATOR + mapping.second->getReference());
                        if (it != defaultValuesForDevice.cend())
                            defaultValue = it->second;
                        m_defaultValueMappingByReference.emplace(key + SEPARATOR + mapping.second->getReference(),
                                                                 defaultValue);
                    }

                    const auto repeatIt = repeatValueMappings.find(mapping.second->getReference());
                    if (repeatIt != repeatValueMappings.cend())
                    {
                        auto repeatValue = repeatIt->second;
                        const auto it = repeatValuesForDevice.find(key + SEPARATOR + mapping.second->getReference());
                        if (it != repeatValuesForDevice.cend())
                        {
                            try
                            {
                                repeatValue = std::chrono::milliseconds(std::stoull(it->second));
                            }
                            catch (const std::exception& exception)
                            {
                                LOG(WARN) << "Found invalid persisted `repeat` value for '" << key << "'/'"
                                          << mapping.second->getReference() << "'.";
                            }
                        }
                        m_repeatedWriteMappingByReference.emplace(key + SEPARATOR + mapping.second->getReference(),
                                                                  repeatValue);

                        mapping.second->setRepeatedWrite(repeatValue);
                    }

                    const auto safeIt = safeMappings.find(mapping.second->getReference());
                    if (safeIt != safeMappings.cend())
                    {
                        auto safeModeValue = safeIt->second;
                        const auto it = safeModeValueForDevice.find(key + SEPARATOR + mapping.second->getReference());
                        if (it != safeModeValueForDevice.cend())
                            safeModeValue = it->second;
                        m_safeModeMappingByReference.emplace(key + SEPARATOR + mapping.second->getReference(),
                                                             safeModeValue);
                    }

                    m_autoReadByReference.emplace(key + SEPARATOR + mapping.second->getReference(),
                                                  autoReadMappings[mapping.second->getReference()]);
                }
            }
        }
    }

    m_modbusReader->addDevices(modbusDevices);
    initializeSetUpDeviceCallback(modbusDevices);
}

bool ModbusBridge::isRunning() const
{
    return m_modbusReader->isRunning();
}

void ModbusBridge::setFeedValueCallback(
  const std::function<void(const std::string&, const std::vector<Reading>&)>& feedValueCallback)
{
    m_feedValueCallback = feedValueCallback;
}

void ModbusBridge::setAttributeCallback(
  const std::function<void(const std::string&, const Attribute&)>& attributeCallback)
{
    m_attributeCallback = attributeCallback;
}

int ModbusBridge::getSlaveAddress(const std::string& deviceKey)
{
    const auto iterator =
      std::find_if(m_deviceKeyBySlaveAddress.begin(), m_deviceKeyBySlaveAddress.end(),
                   [deviceKey](const std::pair<int, std::string>& element) { return element.second == deviceKey; });
    return iterator != m_deviceKeyBySlaveAddress.end() ? iterator->first : -1;
}

// methods for the running logic of modbusBridge
void ModbusBridge::start()
{
    m_modbusReader->start();
    LOG(DEBUG) << "Writing in DefaultValues into mappings.";
    writeAMapOfValues(m_defaultValueMappingByReference);

    // Publish all the DefaultValues, RepeatWriteValues and SafeModeValues
    if (m_feedValueCallback)
    {
        auto readings = std::map<std::string, std::vector<Reading>>{};
        makeReadingsFromMap(readings, m_defaultValueMappingByReference, "DFV");
        makeReadingsFromMap(readings, m_repeatedWriteMappingByReference, "RPW");
        makeReadingsFromMap(readings, m_safeModeMappingByReference, "SMV");
        for (const auto& pair : readings)
            m_feedValueCallback(pair.first, pair.second);
    }
}

void ModbusBridge::stop()
{
    m_modbusReader->stop();
}

void ModbusBridge::platformStatus(ConnectivityStatus status)
{
    LOG(TRACE) << METHOD_INFO;

    // Look if the status has changed
    if (m_connectivityStatus == status)
        return;
    m_connectivityStatus = status;

    // Go through all the safe mode mappings and write their safe mode values in
    if (m_connectivityStatus == ConnectivityStatus::OFFLINE)
    {
        LOG(DEBUG) << "Writing in SafeModeValues into mappings.";
        writeAMapOfValues(m_safeModeMappingByReference);
    }
}

void ModbusBridge::handleUpdate(const std::string& deviceKey,
                                const std::map<std::uint64_t, std::vector<Reading>>& readings)
{
    LOG(TRACE) << METHOD_INFO;

    // Check the device key
    auto slaveAddress = getSlaveAddress(deviceKey);
    if (slaveAddress == -1 || m_deviceKeyBySlaveAddress.find(slaveAddress) == m_deviceKeyBySlaveAddress.end())
    {
        LOG(ERROR) << TAG << "No device with key '" << deviceKey << "'";
        return;
    }

    // Go through the sets of readings
    for (const auto& readingSet : readings)
    {
        // Go through all the readings now
        for (const auto& reading : readingSet.second)
        {
            // Check if it is one of the special feeds
            if (isDefaultValueReading(reading))
            {
                handleDefaultValueReading(deviceKey, reading);
                continue;
            }
            else if (isRepeatWriteReading(reading))
            {
                handleRepeatWriteReading(deviceKey, reading);
                continue;
            }
            else if (isSafeModeValueReading(reading))
            {
                handleSafeModeValueReading(deviceKey, reading);
                continue;
            }

            // Handle it like a normal feed
            const auto mappingIt = m_registerMappingByReference.find(deviceKey + SEPARATOR + reading.getReference());
            if (mappingIt == m_registerMappingByReference.cend())
            {
                LOG(ERROR) << "Received reading for a mapping that could not be found.";
                continue;
            }
            writeToMapping(mappingIt->second, reading.getStringValue());
        }
    }

    // Check what kind of feeds got their values changed
}

void ModbusBridge::handleUpdate(const std::string& deviceKey, const std::vector<Parameter>& parameters)
{
    LOG(TRACE) << METHOD_INFO;

    // Check the device key
    auto slaveAddress = getSlaveAddress(deviceKey);
    if (slaveAddress == -1 || m_deviceKeyBySlaveAddress.find(slaveAddress) == m_deviceKeyBySlaveAddress.end())
    {
        LOG(ERROR) << TAG << "Received parameters update for device '" << deviceKey
                   << "' but the device key was not found.";
        return;
    }

    // For now, we don't have anything to do with the parameters
    for (const auto& parameter : parameters)
        LOG(INFO) << TAG << "Updated parameter for device '" << deviceKey << "' -> Name: '" << toString(parameter.first)
                  << "' | Value: '" << parameter.second << "'";
}

void ModbusBridge::initializeSetUpDeviceCallback(const std::vector<std::shared_ptr<more_modbus::ModbusDevice>>& devices)
{
    // Set up the device mapping value change logic.
    for (const auto& device : devices)
    {
        device->setOnStatusChange(
          [device](bool status)
          {
              LOG(INFO) << "Device status '" << device->getName() << "' changed to '"
                        << (status ? "CONNECTED" : "DISCONNECTED") << "'.";
          });

        device->setOnMappingValueChange([this, device](const std::shared_ptr<more_modbus::RegisterMapping>& mapping,
                                                       const std::vector<std::uint16_t>& bytes)
                                        { sendOutMappingValue(device, mapping, bytes); });

        device->setOnMappingValueChange([this, device](const std::shared_ptr<more_modbus::RegisterMapping>& mapping,
                                                       bool data) { sendOutMappingValue(device, mapping, data); });
    }
}

void ModbusBridge::writeAMapOfValues(const std::map<std::string, std::string>& mapOfValues)
{
    for (const auto& pair : mapOfValues)
    {
        const auto& mapping = m_registerMappingByReference[pair.first];
        writeToMapping(mapping, pair.second);
        if (mapping->getOutputType() == more_modbus::OutputType::BOOL)
            triggerGroupValueChangeBool(mapping);
        else
            triggerGroupValueChangeBytes(mapping);
    }
}

void ModbusBridge::triggerGroupValueChangeBool(const std::shared_ptr<more_modbus::RegisterMapping>& mapping)
{
    if (auto group = mapping->getGroup().lock())
    {
        if (auto device = group->getDevice().lock())
        {
            device->triggerOnMappingValueChange(mapping, mapping->getBoolValue());
            sendOutMappingValue(device, mapping, mapping->getBoolValue());
        }
    }
}

void ModbusBridge::triggerGroupValueChangeBytes(const std::shared_ptr<more_modbus::RegisterMapping>& mapping)
{
    if (auto group = mapping->getGroup().lock())
    {
        if (auto device = group->getDevice().lock())
        {
            device->triggerOnMappingValueChange(mapping, mapping->getBytesValues());
            sendOutMappingValue(device, mapping, mapping->getBytesValues());
        }
    }
}

void ModbusBridge::writeToMapping(const std::shared_ptr<more_modbus::RegisterMapping>& mapping,
                                  const std::string& value)
{
    LOG(TRACE) << TAG << METHOD_INFO;

    try
    {
        switch (mapping->getOutputType())
        {
        case more_modbus::OutputType::BOOL:
        {
            auto boolValue = [&]
            {
                auto valueCopy = std::string{value};
                std::transform(valueCopy.cbegin(), valueCopy.cend(), valueCopy.begin(), ::tolower);
                if (valueCopy == "true")
                    return true;
                else if (valueCopy == "false")
                    return false;
                throw std::runtime_error("The mapping value is not a valid bool value.");
            }();
            writeToMappingSpecific<more_modbus::BoolMapping, bool>(mapping, boolValue);
            return;
        }
        case more_modbus::OutputType::UINT16:
            writeToMappingSpecific<more_modbus::UInt16Mapping, std::uint16_t>(
              mapping, static_cast<std::uint16_t>(std::stoul(value)));
            break;
        case more_modbus::OutputType::INT16:
            writeToMappingSpecific<more_modbus::Int16Mapping, std::int16_t>(
              mapping, static_cast<std::int16_t>(std::stoi(value)));
            break;
        case more_modbus::OutputType::UINT32:
            writeToMappingSpecific<more_modbus::UInt32Mapping, std::uint32_t>(
              mapping, static_cast<std::uint32_t>(std::stoul(value)));
            break;
        case more_modbus::OutputType::INT32:
            writeToMappingSpecific<more_modbus::Int32Mapping, std::int32_t>(mapping, std::stoi(value));
            break;
        case more_modbus::OutputType::FLOAT:
            writeToMappingSpecific<more_modbus::FloatMapping, float>(mapping, std::stof(value));
            break;
        case more_modbus::OutputType::STRING:
            writeToMappingSpecific<more_modbus::StringMapping, std::string>(mapping, value);
            break;
        }
    }
    catch (const std::exception& exception)
    {
        LOG(ERROR) << TAG << "Failed to write in a value into the mapping. The value is not valid -> '"
                   << exception.what() << "'.";
    }
}

void ModbusBridge::sendOutMappingValue(const std::shared_ptr<more_modbus::ModbusDevice>& device,
                                       const std::shared_ptr<more_modbus::RegisterMapping>& mapping,
                                       const std::vector<std::uint16_t>& bytes)
{
    // Find the device key of the device that had a value update
    const auto deviceKeyIt = m_deviceKeyBySlaveAddress.find(device->getSlaveAddress());
    if (deviceKeyIt == m_deviceKeyBySlaveAddress.cend())
    {
        LOG(WARN) << TAG << "Received value update from device with a slave address that is not in the registry.";
        return;
    }
    const auto deviceKey = deviceKeyIt->second;

    // Check that there is a callback
    if (!m_feedValueCallback)
    {
        LOG(WARN) << TAG << "Received value update for '" << deviceKey << "'/'" << mapping->getReference()
                  << "' but the callback is not set.";
        return;
    }

    // Check the type of the mapping
    const auto mappingTypeIt = m_registerMappingTypeByReference.find(deviceKey + SEPARATOR + mapping->getReference());
    if (mappingTypeIt == m_registerMappingTypeByReference.cend())
    {
        LOG(WARN) << TAG << "Received value update for '" << deviceKey << "'/'" << mapping->getReference()
                  << "' but the mapping type for this mapping is unknown.";
        return;
    }

    // Check if it as an attribute
    if (mappingTypeIt->second == MappingType::Attribute)
    {
        // Form the attribute for this value
        const auto attribute = formAttributeForMappingValue(mapping, bytes);
        if (attribute.getName().empty())
        {
            LOG(WARN) << TAG << "Received value update for '" << deviceKey << "'/'" << mapping->getReference()
                      << "' but failed to form the attribute.";
            return;
        }
        m_attributeCallback(deviceKey, attribute);
        return;
    }

    // Form the reading for this value
    const auto reading = formReadingForMappingValue(mapping, bytes);
    if (reading.getReference().empty())
    {
        LOG(WARN) << TAG << "Received value update for '" << deviceKey << "'/'" << mapping->getReference()
                  << "' but failed to form the reading.";
        return;
    }
    m_feedValueCallback(deviceKey, {reading});
}

void ModbusBridge::sendOutMappingValue(const std::shared_ptr<more_modbus::ModbusDevice>& device,
                                       const std::shared_ptr<more_modbus::RegisterMapping>& mapping, bool value)
{
    // Find the device key of the device that had a value update
    const auto deviceKeyIt = m_deviceKeyBySlaveAddress.find(device->getSlaveAddress());
    if (deviceKeyIt == m_deviceKeyBySlaveAddress.cend())
    {
        LOG(WARN) << TAG << "Received value update from device with a slave address that is not in the registry.";
        return;
    }
    const auto deviceKey = deviceKeyIt->second;

    // Check that there is a callback
    if (!m_feedValueCallback)
    {
        LOG(WARN) << TAG << "Received value update for '" << deviceKey << "'/'" << mapping->getReference()
                  << "' but the callback is not set.";
        return;
    }

    // Form the reading for this value and call the callback
    m_feedValueCallback(deviceKey, {Reading{mapping->getReference(), value}});
}

Reading ModbusBridge::formReadingForMappingValue(const std::shared_ptr<more_modbus::RegisterMapping>& mapping,
                                                 const std::vector<std::uint16_t>& bytes)
{
    try
    {
        switch (mapping->getOutputType())
        {
        case more_modbus::OutputType::UINT16:
        {
            const auto uint16Mapping = std::dynamic_pointer_cast<more_modbus::UInt16Mapping>(mapping);
            return {mapping->getReference(), static_cast<std::uint64_t>(uint16Mapping->getValue())};
        }
        case more_modbus::OutputType::INT16:
        {
            const auto int16Mapping = std::dynamic_pointer_cast<more_modbus::Int16Mapping>(mapping);
            return {mapping->getReference(), static_cast<std::int64_t>(int16Mapping->getValue())};
        }
        case more_modbus::OutputType::UINT32:
        {
            const auto uint32Mapping = std::dynamic_pointer_cast<more_modbus::UInt32Mapping>(mapping);
            return {mapping->getReference(), static_cast<std::uint64_t>(uint32Mapping->getValue())};
        }
        case more_modbus::OutputType::INT32:
        {
            const auto int32Mapping = std::dynamic_pointer_cast<more_modbus::Int32Mapping>(mapping);
            return {mapping->getReference(), static_cast<std::int64_t>(int32Mapping->getValue())};
        }
        case more_modbus::OutputType::FLOAT:
        {
            const auto floatMapping = std::dynamic_pointer_cast<more_modbus::FloatMapping>(mapping);
            return {mapping->getReference(), floatMapping->getValue()};
        }
        case more_modbus::OutputType::STRING:
        {
            const auto stringMapping = std::dynamic_pointer_cast<more_modbus::StringMapping>(mapping);
            return {mapping->getReference(), stringMapping->getValue()};
        }
        default:
            return {"", false};
        }
    }
    catch (const std::exception& exception)
    {
        LOG(ERROR) << TAG << "Failed to form a reading for the mapping '" << mapping->getReference() << "' -> '"
                   << exception.what() << "'.";
        return {"", false};
    }
}

Attribute ModbusBridge::formAttributeForMappingValue(const std::shared_ptr<more_modbus::RegisterMapping>& mapping,
                                                     const std::vector<std::uint16_t>& bytes)
{
    try
    {
        switch (mapping->getOutputType())
        {
        case more_modbus::OutputType::UINT16:
            return {mapping->getReference(), DataType::NUMERIC, std::to_string(static_cast<std::uint64_t>(bytes[0]))};
        case more_modbus::OutputType::INT16:
            return {mapping->getReference(), DataType::NUMERIC, std::to_string(static_cast<std::int64_t>(bytes[0]))};
        case more_modbus::OutputType::UINT32:
            switch (mapping->getOperationType())
            {
            case more_modbus::OperationType::MERGE_BIG_ENDIAN:
                return {mapping->getReference(), DataType::NUMERIC,
                        std::to_string(static_cast<std::uint64_t>(
                          more_modbus::DataParsers::registersToUint32(bytes, more_modbus::DataParsers::Endian::BIG)))};
            case more_modbus::OperationType::MERGE_LITTLE_ENDIAN:
                return {mapping->getReference(), DataType::NUMERIC,
                        std::to_string(static_cast<std::uint64_t>(more_modbus::DataParsers::registersToUint32(
                          bytes, more_modbus::DataParsers::Endian::LITTLE)))};
            default:
                return {"", DataType::BOOLEAN, ""};
            }
        case more_modbus::OutputType::INT32:
            switch (mapping->getOperationType())
            {
            case more_modbus::OperationType::MERGE_BIG_ENDIAN:
                return {mapping->getReference(), DataType::NUMERIC,
                        std::to_string(static_cast<std::int64_t>(
                          more_modbus::DataParsers::registersToInt32(bytes, more_modbus::DataParsers::Endian::BIG)))};
            case more_modbus::OperationType::MERGE_LITTLE_ENDIAN:
                return {mapping->getReference(), DataType::NUMERIC,
                        std::to_string(static_cast<std::int64_t>(more_modbus::DataParsers::registersToInt32(
                          bytes, more_modbus::DataParsers::Endian::LITTLE)))};
            default:
                return {"", DataType::BOOLEAN, ""};
            }
        case more_modbus::OutputType::FLOAT:
            switch (mapping->getOperationType())
            {
            case more_modbus::OperationType::MERGE_FLOAT_BIG_ENDIAN:
                return {mapping->getReference(), DataType::NUMERIC,
                        std::to_string(
                          more_modbus::DataParsers::registersToFloat(bytes, more_modbus::DataParsers::Endian::BIG))};
            case more_modbus::OperationType::MERGE_FLOAT_LITTLE_ENDIAN:
                return {mapping->getReference(), DataType::NUMERIC,
                        std::to_string(
                          more_modbus::DataParsers::registersToFloat(bytes, more_modbus::DataParsers::Endian::LITTLE))};
            default:
                return {"", DataType::NUMERIC, ""};
            }
        case more_modbus::OutputType::STRING:
            switch (mapping->getOperationType())
            {
            case more_modbus::OperationType::STRINGIFY_ASCII_BIG_ENDIAN:
                return {mapping->getReference(), DataType::STRING,
                        more_modbus::DataParsers::registersToAsciiString(bytes, more_modbus::DataParsers::Endian::BIG)};
            case more_modbus::OperationType::STRINGIFY_ASCII_LITTLE_ENDIAN:
                return {
                  mapping->getReference(), DataType::STRING,
                  more_modbus::DataParsers::registersToAsciiString(bytes, more_modbus::DataParsers::Endian::LITTLE)};
            case more_modbus::OperationType::STRINGIFY_UNICODE_BIG_ENDIAN:
                return {
                  mapping->getReference(), DataType::STRING,
                  more_modbus::DataParsers::registersToUnicodeString(bytes, more_modbus::DataParsers::Endian::BIG)};
            case more_modbus::OperationType::STRINGIFY_UNICODE_LITTLE_ENDIAN:
                return {
                  mapping->getReference(), DataType::STRING,
                  more_modbus::DataParsers::registersToUnicodeString(bytes, more_modbus::DataParsers::Endian::LITTLE)};
            default:
                return {"", DataType::BOOLEAN, ""};
            }
        default:
            return {"", DataType::BOOLEAN, ""};
        }
    }
    catch (const std::exception& exception)
    {
        LOG(ERROR) << TAG << "Failed to form an attribute for the mapping '" << mapping->getReference() << "' -> '"
                   << exception.what() << "'.";
        return {"", DataType::BOOLEAN, ""};
    }
}

bool ModbusBridge::isDefaultValueReading(const Reading& reading)
{
    const auto& reference = reading.getReference();
    return reference.find("DFV(") == 0 && reference.rfind(')') == reference.length() - 1;
}

void ModbusBridge::handleDefaultValueReading(const std::string& deviceKey, const Reading& reading)
{
    if (!isDefaultValueReading(reading))
        return;

    // We have received a value for a default value
    const auto& reference = reading.getReference();
    const auto ref = reference.substr(reference.find("DFV(") + 4, reference.length() - 5);
    const auto& value = reading.getStringValue();

    // Find all devices that have the same device type and change it for them all!
    m_defaultValueMappingByReference[deviceKey + SEPARATOR + ref] = value;
    m_defaultValuePersistence->storeValue(deviceKey + SEPARATOR + ref, value);
}

bool ModbusBridge::isRepeatWriteReading(const Reading& reading)
{
    const auto& reference = reading.getReference();
    return reference.find("RPW(") == 0 && reference.rfind(')') == reference.length() - 1;
}

void ModbusBridge::handleRepeatWriteReading(const std::string& deviceKey, const Reading& reading)
{
    if (!isRepeatWriteReading(reading))
        return;

    // We have received a value for a repeated write
    const auto& reference = reading.getReference();
    const auto ref = reference.substr(reference.find("RPW(") + 4, reference.length() - 5);

    try
    {
        // Check if the value can be parsed
        const auto value = reading.getUIntValue();
        const auto milliseconds = std::chrono::milliseconds{};

        // Find all devices that have the same device type and change it for them all!
        m_repeatedWriteMappingByReference[deviceKey + SEPARATOR + ref] = milliseconds;
        m_registerMappingByReference[deviceKey + SEPARATOR + ref]->setRepeatedWrite(milliseconds);
        m_repeatValuePersistence->storeValue(deviceKey + SEPARATOR + ref, std::to_string(value));
    }
    catch (const std::exception& exception)
    {
        LOG(ERROR) << "Failed to accept a new `repeat` value for `" << deviceKey << "`/`" << ref
                   << "` - The value is not a valid number.";
        return;
    }
}

bool ModbusBridge::isSafeModeValueReading(const Reading& reading)
{
    const auto& reference = reading.getReference();
    return reference.find("SMV(") == 0 && reference.rfind(')') == reference.length() - 1;
}

void ModbusBridge::handleSafeModeValueReading(const std::string& deviceKey, const Reading& reading)
{
    if (!isSafeModeValueReading(reading))
        return;

    // We have received a value for a safe mode value
    const auto& reference = reading.getReference();
    const auto ref = reference.substr(reference.find("SMV(") + 4, reference.length() - 5);
    const auto& value = reading.getStringValue();

    // Find all devices that have the same device type and change it for them all!
    m_safeModeMappingByReference[deviceKey + SEPARATOR + ref] = value;
    m_safeModePersistence->storeValue(deviceKey + SEPARATOR + ref, value);
}
}    // namespace wolkabout::modbus
