#ifndef MOCKDATAPROTOCOL_H
#define MOCKDATAPROTOCOL_H

#include "model/ActuatorGetCommand.h"
#include "model/ActuatorSetCommand.h"
#include "model/ConfigurationSetCommand.h"
#include "protocol/DataProtocol.h"

#include <gmock/gmock.h>

class MockDataProtocol : public wolkabout::DataProtocol
{
public:
    MOCK_CONST_METHOD1(extractReferenceFromChannel, std::string(const std::string&));

    const std::string& getName() const override { return m_name; }

    std::vector<std::string> getInboundChannels() const override { return m_channels; };
    std::vector<std::string> getInboundChannelsForDevice(const std::string& deviceKey) const override
    {
        return m_channels;
    };

    MOCK_CONST_METHOD1(extractDeviceKeyFromChannel, std::string(const std::string&));

    MOCK_CONST_METHOD1(isActuatorSetMessage, bool(const wolkabout::Message& message));

    MOCK_CONST_METHOD1(isActuatorGetMessage, bool(const wolkabout::Message& message));

    MOCK_CONST_METHOD1(isConfigurationSetMessage, bool(const wolkabout::Message& message));

    MOCK_CONST_METHOD1(isConfigurationGetMessage, bool(const wolkabout::Message& message));

    std::unique_ptr<wolkabout::ActuatorGetCommand> makeActuatorGetCommand(
      const wolkabout::Message& message) const override
    {
        return std::unique_ptr<wolkabout::ActuatorGetCommand>(makeActuatorGetCommandProxy(message));
    }

    MOCK_CONST_METHOD1(makeActuatorGetCommandProxy, wolkabout::ActuatorGetCommand*(const wolkabout::Message& message));

    std::unique_ptr<wolkabout::ActuatorSetCommand> makeActuatorSetCommand(
      const wolkabout::Message& message) const override
    {
        return std::unique_ptr<wolkabout::ActuatorSetCommand>(makeActuatorSetCommandProxy(message));
    }

    MOCK_CONST_METHOD1(makeActuatorSetCommandProxy, wolkabout::ActuatorSetCommand*(const wolkabout::Message& message));

    std::unique_ptr<wolkabout::ConfigurationSetCommand> makeConfigurationSetCommand(
      const wolkabout::Message& message, const std::map<std::string, std::string>& delimiters) const override
    {
        return std::unique_ptr<wolkabout::ConfigurationSetCommand>(
          makeConfigurationSetCommandProxy(message, delimiters));
    }

    MOCK_CONST_METHOD2(makeConfigurationSetCommandProxy,
                       wolkabout::ConfigurationSetCommand*(const wolkabout::Message& message,
                                                           const std::map<std::string, std::string>& delimiters));

    std::unique_ptr<wolkabout::Message> makeMessage(
      const std::string& deviceKey, const std::vector<std::shared_ptr<wolkabout::SensorReading>>& sensorReadings,
      const std::string& delimiter) const override
    {
        return std::unique_ptr<wolkabout::Message>(makeMessageProxy(deviceKey, sensorReadings, delimiter));
    }

    MOCK_CONST_METHOD3(makeMessageProxy,
                       wolkabout::Message*(const std::string& deviceKey,
                                           const std::vector<std::shared_ptr<wolkabout::SensorReading>>& sensorReadings,
                                           const std::string& delimiter));

    std::unique_ptr<wolkabout::Message> makeMessage(
      const std::string& deviceKey, const std::vector<std::shared_ptr<wolkabout::Alarm>>& alarms) const override
    {
        return std::unique_ptr<wolkabout::Message>(makeMessageProxy(deviceKey, alarms));
    }

    MOCK_CONST_METHOD2(makeMessageProxy,
                       wolkabout::Message*(const std::string& deviceKey,
                                           const std::vector<std::shared_ptr<wolkabout::Alarm>>& alarms));

    std::unique_ptr<wolkabout::Message> makeMessage(
      const std::string& deviceKey,
      const std::vector<std::shared_ptr<wolkabout::ActuatorStatus>>& actuatorStatuses) const override
    {
        return std::unique_ptr<wolkabout::Message>(makeMessageProxy(deviceKey, actuatorStatuses));
    }

    MOCK_CONST_METHOD2(
      makeMessageProxy,
      wolkabout::Message*(const std::string& deviceKey,
                          const std::vector<std::shared_ptr<wolkabout::ActuatorStatus>>& actuatorStatuses));

    std::unique_ptr<wolkabout::Message> makeMessage(const std::string& deviceKey,
                                                    const std::vector<wolkabout::ConfigurationItem>& configuration,
                                                    const std::map<std::string, std::string>& delimiters) const override
    {
        return std::unique_ptr<wolkabout::Message>(makeMessageProxy(deviceKey, configuration, delimiters));
    }

    MOCK_CONST_METHOD3(makeMessageProxy,
                       wolkabout::Message*(const std::string& deviceKey,
                                           const std::vector<wolkabout::ConfigurationItem>& configuration,
                                           const std::map<std::string, std::string>& delimiters));

private:
    const std::string m_name{""};
    const std::vector<std::string> m_channels{};
};

#endif    // MOCKDATAPROTOCOL_H
