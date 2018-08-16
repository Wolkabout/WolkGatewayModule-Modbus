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

#include "MockDataProtocol.h"
#include "MockPersistance.h"
#include "connectivity/ConnectivityService.h"
#include "model/Message.h"

#define private public
#define protected public
#include "service/DataService.h"
#undef private
#undef protected

#include <gtest/gtest.h>
#include <cstdio>
#include <memory>
#include <tuple>
#include <vector>

namespace
{
class ConnectivityService : public wolkabout::ConnectivityService
{
public:
    bool connect() override { return true; }
    void disconnect() override {}

    bool reconnect() override { return true; }

    bool isConnected() override { return true; }

    bool publish(std::shared_ptr<wolkabout::Message> message, bool persistent) override
    {
        m_messages.push_back(message);
        return true;
    }

    void setUncontrolledDisonnectMessage(std::shared_ptr<wolkabout::Message> outboundMessage, bool persistent) override
    {
    }

    const std::vector<std::shared_ptr<wolkabout::Message>>& getMessages() const { return m_messages; }

private:
    std::vector<std::shared_ptr<wolkabout::Message>> m_messages;
};

class DataService : public ::testing::Test
{
public:
    void SetUp() override
    {
        dataProtocol = std::unique_ptr<MockDataProtocol>(new MockDataProtocol());
        persistence = std::unique_ptr<MockPersistence>(new MockPersistence());
        connectivityService = std::unique_ptr<ConnectivityService>(new ConnectivityService());

        dataService = std::unique_ptr<wolkabout::DataService>(new wolkabout::DataService(
          *dataProtocol, *persistence, *connectivityService,
          [&](const std::string& key, const std::string& ref, const std::string& value) {
              actuatorSetCommands.push_back(std::make_tuple(key, ref, value));
          },
          [&](const std::string& key, const std::string& ref) {
              actuatorGetCommands.push_back(std::make_tuple(key, ref));
          },
          [&](const std::string& key, const std::vector<wolkabout::ConfigurationItem>& values) {
              configurationSetCommands.push_back(std::make_tuple(key, values));
          },
          [&](const std::string& key) { configurationGetCommands.push_back(key); }));
    }

    void TearDown() override {}

    std::unique_ptr<MockDataProtocol> dataProtocol;
    std::unique_ptr<MockPersistence> persistence;
    std::unique_ptr<ConnectivityService> connectivityService;

    std::vector<std::tuple<std::string, std::string, std::string>> actuatorSetCommands;
    std::vector<std::tuple<std::string, std::string>> actuatorGetCommands;
    std::vector<std::tuple<std::string, std::vector<wolkabout::ConfigurationItem>>> configurationSetCommands;
    std::vector<std::string> configurationGetCommands;

    std::unique_ptr<wolkabout::DataService> dataService;
};
}    // namespace

TEST_F(DataService, Given_When_MessageWithInvalidChannelKeyIsReceived_Then_MessageIsIgnored)
{
    // Given
    EXPECT_CALL(*dataProtocol, extractDeviceKeyFromChannel(testing::_)).Times(1).WillOnce(testing::Return(""));

    // When
    auto message = std::make_shared<wolkabout::Message>("", "");
    dataService->messageReceived(message);

    // Then
    ASSERT_TRUE(actuatorSetCommands.empty());
    ASSERT_TRUE(actuatorGetCommands.empty());
    ASSERT_TRUE(configurationSetCommands.empty());
    ASSERT_TRUE(configurationGetCommands.empty());
}

TEST_F(DataService, Given_When_ActuatorGetMessageIsReceived_Then_ActuationGetHandlerIsCalled)
{
    // Given
    EXPECT_CALL(*dataProtocol, extractDeviceKeyFromChannel(testing::_))
      .Times(1)
      .WillOnce(testing::Return("DEVICE_KEY"));

    EXPECT_CALL(*dataProtocol, isActuatorGetMessage(testing::_)).Times(1).WillOnce(testing::Return(true));

    EXPECT_CALL(*dataProtocol, makeActuatorGetCommandProxy(testing::_))
      .Times(1)
      .WillOnce(testing::Return(new wolkabout::ActuatorGetCommand()));

    ON_CALL(*dataProtocol, isActuatorSetMessage(testing::_)).WillByDefault(testing::Return(false));
    ON_CALL(*dataProtocol, isConfigurationSetMessage(testing::_)).WillByDefault(testing::Return(false));
    ON_CALL(*dataProtocol, isConfigurationGetMessage(testing::_)).WillByDefault(testing::Return(false));

    // When
    auto message = std::make_shared<wolkabout::Message>("", "");
    dataService->messageReceived(message);

    // Then
    ASSERT_EQ(actuatorGetCommands.size(), 1);
    ASSERT_TRUE(actuatorSetCommands.empty());
    ASSERT_TRUE(configurationSetCommands.empty());
    ASSERT_TRUE(configurationGetCommands.empty());
}

TEST_F(DataService, Given_When_InvalidActuatorGetMessageIsReceived_Then_MessageIsIgnored)
{
    // Given
    EXPECT_CALL(*dataProtocol, extractDeviceKeyFromChannel(testing::_))
      .Times(1)
      .WillOnce(testing::Return("DEVICE_KEY"));

    EXPECT_CALL(*dataProtocol, isActuatorGetMessage(testing::_)).Times(1).WillOnce(testing::Return(true));

    EXPECT_CALL(*dataProtocol, makeActuatorGetCommandProxy(testing::_)).Times(1).WillOnce(testing::Return(nullptr));

    ON_CALL(*dataProtocol, isActuatorSetMessage(testing::_)).WillByDefault(testing::Return(false));
    ON_CALL(*dataProtocol, isConfigurationSetMessage(testing::_)).WillByDefault(testing::Return(false));
    ON_CALL(*dataProtocol, isConfigurationGetMessage(testing::_)).WillByDefault(testing::Return(false));

    // When
    auto message = std::make_shared<wolkabout::Message>("", "");
    dataService->messageReceived(message);

    // Then
    ASSERT_TRUE(actuatorGetCommands.empty());
    ASSERT_TRUE(actuatorSetCommands.empty());
    ASSERT_TRUE(configurationSetCommands.empty());
    ASSERT_TRUE(configurationGetCommands.empty());
}

TEST_F(DataService, Given_When_ActuatorSetMessageIsReceived_Then_ActuationSetHandlerIsCalled)
{
    // Given
    EXPECT_CALL(*dataProtocol, extractDeviceKeyFromChannel(testing::_))
      .Times(1)
      .WillOnce(testing::Return("DEVICE_KEY"));

    EXPECT_CALL(*dataProtocol, isActuatorSetMessage(testing::_)).Times(1).WillOnce(testing::Return(true));

    EXPECT_CALL(*dataProtocol, makeActuatorSetCommandProxy(testing::_))
      .Times(1)
      .WillOnce(testing::Return(new wolkabout::ActuatorSetCommand()));

    ON_CALL(*dataProtocol, isActuatorGetMessage(testing::_)).WillByDefault(testing::Return(false));
    ON_CALL(*dataProtocol, isConfigurationSetMessage(testing::_)).WillByDefault(testing::Return(false));
    ON_CALL(*dataProtocol, isConfigurationGetMessage(testing::_)).WillByDefault(testing::Return(false));

    // When
    auto message = std::make_shared<wolkabout::Message>("", "");
    dataService->messageReceived(message);

    // Then
    ASSERT_TRUE(actuatorGetCommands.empty());
    ASSERT_EQ(actuatorSetCommands.size(), 1);
    ASSERT_TRUE(configurationSetCommands.empty());
    ASSERT_TRUE(configurationGetCommands.empty());
}

TEST_F(DataService, Given_When_InvalidActuatorSetMessageIsReceived_Then_MessageIsIgnored)
{
    // Given
    EXPECT_CALL(*dataProtocol, extractDeviceKeyFromChannel(testing::_))
      .Times(1)
      .WillOnce(testing::Return("DEVICE_KEY"));

    EXPECT_CALL(*dataProtocol, isActuatorSetMessage(testing::_)).Times(1).WillOnce(testing::Return(true));

    EXPECT_CALL(*dataProtocol, makeActuatorSetCommandProxy(testing::_)).Times(1).WillOnce(testing::Return(nullptr));

    ON_CALL(*dataProtocol, isActuatorGetMessage(testing::_)).WillByDefault(testing::Return(false));
    ON_CALL(*dataProtocol, isConfigurationSetMessage(testing::_)).WillByDefault(testing::Return(false));
    ON_CALL(*dataProtocol, isConfigurationGetMessage(testing::_)).WillByDefault(testing::Return(false));

    // When
    auto message = std::make_shared<wolkabout::Message>("", "");
    dataService->messageReceived(message);

    // Then
    ASSERT_TRUE(actuatorGetCommands.empty());
    ASSERT_TRUE(actuatorSetCommands.empty());
    ASSERT_TRUE(configurationSetCommands.empty());
    ASSERT_TRUE(configurationGetCommands.empty());
}

TEST_F(DataService, Given_When_ConfigurationGetMessageIsReceived_Then_ConfigurationGetHandlerIsCalled)
{
    // Given
    EXPECT_CALL(*dataProtocol, extractDeviceKeyFromChannel(testing::_))
      .Times(1)
      .WillOnce(testing::Return("DEVICE_KEY"));

    EXPECT_CALL(*dataProtocol, isConfigurationGetMessage(testing::_)).Times(1).WillOnce(testing::Return(true));

    // TODO add makeConfigurationGetCommand to data protocol api
    //    EXPECT_CALL(*dataProtocol, makeConfigurationGetCommandProxy(testing::_))
    //            .Times(1)
    //            .WillOnce(testing::Return(new wolkabout::ActuatorGetCommand()));

    ON_CALL(*dataProtocol, isActuatorGetMessage(testing::_)).WillByDefault(testing::Return(false));
    ON_CALL(*dataProtocol, isActuatorSetMessage(testing::_)).WillByDefault(testing::Return(false));
    ON_CALL(*dataProtocol, isConfigurationSetMessage(testing::_)).WillByDefault(testing::Return(false));

    // When
    auto message = std::make_shared<wolkabout::Message>("", "");
    dataService->messageReceived(message);

    // Then
    ASSERT_TRUE(actuatorGetCommands.empty());
    ASSERT_TRUE(actuatorSetCommands.empty());
    ASSERT_TRUE(configurationSetCommands.empty());
    ASSERT_EQ(configurationGetCommands.size(), 1);
}

TEST_F(DataService, Given_When_ConfigurationSetMessageIsReceived_Then_ConfigurationSetHandlerIsCalled)
{
    // Given
    EXPECT_CALL(*dataProtocol, extractDeviceKeyFromChannel(testing::_))
      .Times(1)
      .WillOnce(testing::Return("DEVICE_KEY"));

    EXPECT_CALL(*dataProtocol, isConfigurationSetMessage(testing::_)).Times(1).WillOnce(testing::Return(true));

    EXPECT_CALL(*dataProtocol, makeConfigurationSetCommandProxy(testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Return(new wolkabout::ConfigurationSetCommand({})));

    ON_CALL(*dataProtocol, isActuatorGetMessage(testing::_)).WillByDefault(testing::Return(false));
    ON_CALL(*dataProtocol, isActuatorSetMessage(testing::_)).WillByDefault(testing::Return(false));
    ON_CALL(*dataProtocol, isConfigurationGetMessage(testing::_)).WillByDefault(testing::Return(false));

    // When
    auto message = std::make_shared<wolkabout::Message>("", "");
    dataService->messageReceived(message);

    // Then
    ASSERT_TRUE(actuatorGetCommands.empty());
    ASSERT_TRUE(actuatorSetCommands.empty());
    ASSERT_EQ(configurationSetCommands.size(), 1);
    ASSERT_TRUE(configurationGetCommands.empty());
}

TEST_F(DataService, Given_When_InvalidConfigurationSetMessageIsReceived_Then_MessageIsIgnored)
{
    // Given
    EXPECT_CALL(*dataProtocol, extractDeviceKeyFromChannel(testing::_))
      .Times(1)
      .WillOnce(testing::Return("DEVICE_KEY"));

    EXPECT_CALL(*dataProtocol, isConfigurationSetMessage(testing::_)).Times(1).WillOnce(testing::Return(true));

    EXPECT_CALL(*dataProtocol, makeConfigurationSetCommandProxy(testing::_, testing::_))
      .Times(1)
      .WillOnce(testing::Return(nullptr));

    ON_CALL(*dataProtocol, isActuatorGetMessage(testing::_)).WillByDefault(testing::Return(false));
    ON_CALL(*dataProtocol, isActuatorSetMessage(testing::_)).WillByDefault(testing::Return(false));
    ON_CALL(*dataProtocol, isConfigurationGetMessage(testing::_)).WillByDefault(testing::Return(false));

    // When
    auto message = std::make_shared<wolkabout::Message>("", "");
    dataService->messageReceived(message);

    // Then
    ASSERT_TRUE(actuatorGetCommands.empty());
    ASSERT_TRUE(actuatorSetCommands.empty());
    ASSERT_TRUE(configurationSetCommands.empty());
    ASSERT_TRUE(configurationGetCommands.empty());
}

TEST_F(DataService, Given_SensorReading_When_AddSensorReadingIsCalled_Then_SensorReadingIsAddedToPeristance)
{
    // Given
    const std::string key = "DEVICE_KEY";
    const std::string ref = "REF";
    const std::string value = "VALUE";
    const auto rtc = 2463477347;

    EXPECT_CALL(*persistence, putSensorReading(key + "+" + ref, testing::_)).Times(1).WillOnce(testing::Return(true));

    // When
    dataService->addSensorReading(key, ref, value, rtc);
}

TEST_F(
  DataService,
  Given_MultiValueSensorReading_When_AddSensorReadingIsCalled_Then_SensorReadingIsAddedToPeristanceAndDelimiterIsStored)
{
    // Given
    const std::string key = "DEVICE_KEY";
    const std::string ref = "REF";
    const std::vector<std::string> values = {"VALUE1", "VALUE2"};
    const std::string delimiter = "#";
    const auto rtc = 2463477347;

    EXPECT_CALL(*persistence, putSensorReading(key + "+" + ref, testing::_)).Times(1).WillOnce(testing::Return(true));

    // When
    dataService->addSensorReading(key, ref, values, delimiter, rtc);

    // Then
    ASSERT_EQ(dataService->getSensorDelimiter(key + "+" + ref), delimiter);
}

TEST_F(DataService, Given_Alarm_When_AddAlarmIsCalled_Then_AlarmIsAddedToPeristance)
{
    // Given
    const std::string key = "DEVICE_KEY";
    const std::string ref = "REF";
    const bool value = true;
    const auto rtc = 2463477347;

    EXPECT_CALL(*persistence, putAlarm(key + "+" + ref, testing::_)).Times(1).WillOnce(testing::Return(true));

    // When
    dataService->addAlarm(key, ref, value, rtc);
}

TEST_F(DataService, Given_ActuatorStatus_When_AddActuatorStatusIsCalled_Then_ActuatorStatusIsAddedToPeristance)
{
    // Given
    const std::string key = "DEVICE_KEY";
    const std::string ref = "REF";
    const std::string value = "VALUE";
    const wolkabout::ActuatorStatus::State status = wolkabout::ActuatorStatus::State::READY;

    EXPECT_CALL(*persistence, putActuatorStatus(key + "+" + ref, testing::_)).Times(1).WillOnce(testing::Return(true));

    // When
    dataService->addActuatorStatus(key, ref, value, status);
}

TEST_F(DataService, Given_Configuration_When_AddConfigurationIsCalled_Then_ConfigurationIsAddedToPeristance)
{
    // Given
    const std::string key = "DEVICE_KEY";
    const std::string ref = "REF";
    const std::vector<wolkabout::ConfigurationItem> values = {{{"VALUE"}, "KEY"}};

    EXPECT_CALL(*persistence, putConfiguration(key, testing::_)).Times(1).WillOnce(testing::Return(true));

    // When
    dataService->addConfiguration(key, values, {});
}

TEST_F(
  DataService,
  Given_PersistedSensorReadings_When_PublishSensorReadingsForAllIsCalled_Then_ReadingsArePublishedAndPersistenceIsEmptied)
{
    // Given
    const auto key1 = "KEY1+REF1";
    const auto key2 = "KEY1+REF2";
    const auto key3 = "KEY2+REF";

    const std::vector<std::shared_ptr<wolkabout::SensorReading>> readings1 = {
      std::make_shared<wolkabout::SensorReading>("VAL", "REF1"),
      std::make_shared<wolkabout::SensorReading>("VAL2", "REF1"),
      std::make_shared<wolkabout::SensorReading>("VAL", "REF1"),
      std::make_shared<wolkabout::SensorReading>("VAL3", "REF1"),
    };

    const std::vector<std::shared_ptr<wolkabout::SensorReading>> readings2 = {
      std::make_shared<wolkabout::SensorReading>("VAL", "REF2")};

    const std::vector<std::shared_ptr<wolkabout::SensorReading>> readings3 = {
      std::make_shared<wolkabout::SensorReading>("VAL", "REF"),
      std::make_shared<wolkabout::SensorReading>("VAL2", "REF"),
      std::make_shared<wolkabout::SensorReading>("VAL", "REF"),
      std::make_shared<wolkabout::SensorReading>("VAL3", "REF")};

    bool removeCalledForKey1 = false;
    bool removeCalledForKey2 = false;
    bool removeCalledForKey3 = false;

    EXPECT_CALL(
      *dataProtocol,
      makeMessageProxy(testing::_,
                       testing::Matcher<const std::vector<std::shared_ptr<wolkabout::SensorReading>>&>(testing::_),
                       testing::_))
      .Times(3)
      .WillRepeatedly(testing::InvokeWithoutArgs([&] { return new wolkabout::Message("", ""); }));

    EXPECT_CALL(*persistence, getSensorReadingsKeys())
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs([&] {
          std::vector<std::string> keys;
          if (!removeCalledForKey1)
              keys.push_back(key1);
          if (!removeCalledForKey2)
              keys.push_back(key2);
          if (!removeCalledForKey3)
              keys.push_back(key3);
          return keys;
      }));

    EXPECT_CALL(*persistence, removeSensorReadings(key1, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT))
      .Times(1)
      .WillOnce(testing::Assign(&removeCalledForKey1, true));
    EXPECT_CALL(*persistence, removeSensorReadings(key2, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT))
      .Times(1)
      .WillOnce(testing::Assign(&removeCalledForKey2, true));
    EXPECT_CALL(*persistence, removeSensorReadings(key3, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT))
      .Times(1)
      .WillOnce(testing::Assign(&removeCalledForKey3, true));

    EXPECT_CALL(*persistence, getSensorReadings(key1, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs(
        [&] { return removeCalledForKey1 ? std::vector<std::shared_ptr<wolkabout::SensorReading>>{} : readings1; }));

    EXPECT_CALL(*persistence, getSensorReadings(key2, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs(
        [&] { return removeCalledForKey2 ? std::vector<std::shared_ptr<wolkabout::SensorReading>>{} : readings2; }));

    EXPECT_CALL(*persistence, getSensorReadings(key3, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs(
        [&] { return removeCalledForKey3 ? std::vector<std::shared_ptr<wolkabout::SensorReading>>{} : readings3; }));

    // When
    dataService->publishSensorReadings();

    ASSERT_EQ(connectivityService->getMessages().size(), 3);
}

TEST_F(
  DataService,
  Given_PersistedSensorReadings_When_PublishSensorReadingsForDeviceKeyIsCalled_Then_ReadingsArePublishedAndPersistenceIsEmptied)
{
    // Given
    const auto key1 = "KEY1+REF1";
    const auto key2 = "KEY1+REF2";
    const auto key3 = "KEY2+REF";

    const std::vector<std::shared_ptr<wolkabout::SensorReading>> readings1 = {
      std::make_shared<wolkabout::SensorReading>("VAL", "REF1"),
      std::make_shared<wolkabout::SensorReading>("VAL2", "REF1"),
      std::make_shared<wolkabout::SensorReading>("VAL", "REF1"),
      std::make_shared<wolkabout::SensorReading>("VAL3", "REF1"),
    };

    const std::vector<std::shared_ptr<wolkabout::SensorReading>> readings2 = {
      std::make_shared<wolkabout::SensorReading>("VAL", "REF2")};

    const std::vector<std::shared_ptr<wolkabout::SensorReading>> readings3 = {
      std::make_shared<wolkabout::SensorReading>("VAL", "REF"),
      std::make_shared<wolkabout::SensorReading>("VAL2", "REF"),
      std::make_shared<wolkabout::SensorReading>("VAL", "REF"),
      std::make_shared<wolkabout::SensorReading>("VAL3", "REF")};

    bool removeCalledForKey1 = false;
    bool removeCalledForKey2 = false;
    // bool removeCalledForKey3 = false;

    EXPECT_CALL(
      *dataProtocol,
      makeMessageProxy(testing::_,
                       testing::Matcher<const std::vector<std::shared_ptr<wolkabout::SensorReading>>&>(testing::_),
                       testing::_))
      .Times(2)
      .WillRepeatedly(testing::InvokeWithoutArgs([&] { return new wolkabout::Message("", ""); }));

    EXPECT_CALL(*persistence, getSensorReadingsKeys())
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs([&] {
          std::vector<std::string> keys;
          if (!removeCalledForKey1)
              keys.push_back(key1);
          if (!removeCalledForKey2)
              keys.push_back(key2);
          // if(!removeCalledForKey3)
          keys.push_back(key3);
          return keys;
      }));

    EXPECT_CALL(*persistence, removeSensorReadings(key1, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT))
      .Times(1)
      .WillOnce(testing::Assign(&removeCalledForKey1, true));
    EXPECT_CALL(*persistence, removeSensorReadings(key2, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT))
      .Times(1)
      .WillOnce(testing::Assign(&removeCalledForKey2, true));
    EXPECT_CALL(*persistence, removeSensorReadings(key3, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT)).Times(0);

    EXPECT_CALL(*persistence, getSensorReadings(key1, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs(
        [&] { return removeCalledForKey1 ? std::vector<std::shared_ptr<wolkabout::SensorReading>>{} : readings1; }));

    EXPECT_CALL(*persistence, getSensorReadings(key2, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs(
        [&] { return removeCalledForKey2 ? std::vector<std::shared_ptr<wolkabout::SensorReading>>{} : readings2; }));

    EXPECT_CALL(*persistence, getSensorReadings(key3, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT)).Times(0);

    // When
    dataService->publishSensorReadings("KEY1");

    ASSERT_EQ(connectivityService->getMessages().size(), 2);
}

TEST_F(DataService,
       Given_PersistedAlarms_When_PublishAlarmsForAllIsCalled_Then_AlarmsArePublishedAndPersistenceIsEmptied)
{
    // Given
    const auto key1 = "KEY1+REF1";
    const auto key2 = "KEY1+REF2";
    const auto key3 = "KEY2+REF";

    const std::vector<std::shared_ptr<wolkabout::Alarm>> readings1 = {
      std::make_shared<wolkabout::Alarm>("VAL", "REF1"),
      std::make_shared<wolkabout::Alarm>("VAL2", "REF1"),
      std::make_shared<wolkabout::Alarm>("VAL", "REF1"),
      std::make_shared<wolkabout::Alarm>("VAL3", "REF1"),
    };

    const std::vector<std::shared_ptr<wolkabout::Alarm>> readings2 = {
      std::make_shared<wolkabout::Alarm>("VAL", "REF2")};

    const std::vector<std::shared_ptr<wolkabout::Alarm>> readings3 = {
      std::make_shared<wolkabout::Alarm>("VAL", "REF"), std::make_shared<wolkabout::Alarm>("VAL2", "REF"),
      std::make_shared<wolkabout::Alarm>("VAL", "REF"), std::make_shared<wolkabout::Alarm>("VAL3", "REF")};

    bool removeCalledForKey1 = false;
    bool removeCalledForKey2 = false;
    bool removeCalledForKey3 = false;

    EXPECT_CALL(
      *dataProtocol,
      makeMessageProxy(testing::_, testing::Matcher<const std::vector<std::shared_ptr<wolkabout::Alarm>>&>(testing::_)))
      .Times(3)
      .WillRepeatedly(testing::InvokeWithoutArgs([&] { return new wolkabout::Message("", ""); }));

    EXPECT_CALL(*persistence, getAlarmsKeys())
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs([&] {
          std::vector<std::string> keys;
          if (!removeCalledForKey1)
              keys.push_back(key1);
          if (!removeCalledForKey2)
              keys.push_back(key2);
          if (!removeCalledForKey3)
              keys.push_back(key3);
          return keys;
      }));

    EXPECT_CALL(*persistence, removeAlarms(key1, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT))
      .Times(1)
      .WillOnce(testing::Assign(&removeCalledForKey1, true));
    EXPECT_CALL(*persistence, removeAlarms(key2, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT))
      .Times(1)
      .WillOnce(testing::Assign(&removeCalledForKey2, true));
    EXPECT_CALL(*persistence, removeAlarms(key3, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT))
      .Times(1)
      .WillOnce(testing::Assign(&removeCalledForKey3, true));

    EXPECT_CALL(*persistence, getAlarms(key1, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs(
        [&] { return removeCalledForKey1 ? std::vector<std::shared_ptr<wolkabout::Alarm>>{} : readings1; }));

    EXPECT_CALL(*persistence, getAlarms(key2, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs(
        [&] { return removeCalledForKey2 ? std::vector<std::shared_ptr<wolkabout::Alarm>>{} : readings2; }));

    EXPECT_CALL(*persistence, getAlarms(key3, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs(
        [&] { return removeCalledForKey3 ? std::vector<std::shared_ptr<wolkabout::Alarm>>{} : readings3; }));

    // When
    dataService->publishAlarms();

    ASSERT_EQ(connectivityService->getMessages().size(), 3);
}

TEST_F(DataService,
       Given_PersistedAlarms_When_PublishAlarmsForDeviceKeyIsCalled_Then_AlarmsArePublishedAndPersistenceIsEmptied)
{
    // Given
    const auto key1 = "KEY1+REF1";
    const auto key2 = "KEY1+REF2";
    const auto key3 = "KEY2+REF";

    const std::vector<std::shared_ptr<wolkabout::Alarm>> readings1 = {
      std::make_shared<wolkabout::Alarm>("VAL", "REF1"),
      std::make_shared<wolkabout::Alarm>("VAL2", "REF1"),
      std::make_shared<wolkabout::Alarm>("VAL", "REF1"),
      std::make_shared<wolkabout::Alarm>("VAL3", "REF1"),
    };

    const std::vector<std::shared_ptr<wolkabout::Alarm>> readings2 = {
      std::make_shared<wolkabout::Alarm>("VAL", "REF2")};

    const std::vector<std::shared_ptr<wolkabout::Alarm>> readings3 = {
      std::make_shared<wolkabout::Alarm>("VAL", "REF"), std::make_shared<wolkabout::Alarm>("VAL2", "REF"),
      std::make_shared<wolkabout::Alarm>("VAL", "REF"), std::make_shared<wolkabout::Alarm>("VAL3", "REF")};

    // bool removeCalledForKey1 = false;
    // bool removeCalledForKey2 = false;
    bool removeCalledForKey3 = false;

    EXPECT_CALL(
      *dataProtocol,
      makeMessageProxy(testing::_, testing::Matcher<const std::vector<std::shared_ptr<wolkabout::Alarm>>&>(testing::_)))
      .Times(1)
      .WillRepeatedly(testing::InvokeWithoutArgs([&] { return new wolkabout::Message("", ""); }));

    EXPECT_CALL(*persistence, getAlarmsKeys())
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs([&] {
          std::vector<std::string> keys;
          // if(!removeCalledForKey1)
          keys.push_back(key1);
          // if(!removeCalledForKey2)
          keys.push_back(key2);
          if (!removeCalledForKey3)
              keys.push_back(key3);
          return keys;
      }));

    EXPECT_CALL(*persistence, removeAlarms(key1, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT)).Times(0);
    EXPECT_CALL(*persistence, removeAlarms(key2, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT)).Times(0);
    EXPECT_CALL(*persistence, removeAlarms(key3, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT))
      .Times(1)
      .WillOnce(testing::Assign(&removeCalledForKey3, true));
    ;

    EXPECT_CALL(*persistence, getAlarms(key1, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT)).Times(0);

    EXPECT_CALL(*persistence, getAlarms(key2, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT)).Times(0);

    EXPECT_CALL(*persistence, getAlarms(key3, wolkabout::DataService::PUBLISH_BATCH_ITEMS_COUNT))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs(
        [&] { return removeCalledForKey3 ? std::vector<std::shared_ptr<wolkabout::Alarm>>{} : readings3; }));

    // When
    dataService->publishAlarms("KEY2");

    ASSERT_EQ(connectivityService->getMessages().size(), 1);
}

TEST_F(
  DataService,
  Given_PersistedActuatorStatus_When_PublishActuatorStatusForAllIsCalled_Then_ActuatorStatusArePublishedAndPersistenceIsEmptied)
{
    // Given
    const auto key1 = "KEY1+REF1";
    const auto key2 = "KEY1+REF2";
    const auto key3 = "KEY2+REF";

    const std::shared_ptr<wolkabout::ActuatorStatus> status1 =
      std::make_shared<wolkabout::ActuatorStatus>("VAL", "REF1", wolkabout::ActuatorStatus::State::READY);

    const std::shared_ptr<wolkabout::ActuatorStatus> status2 =
      std::make_shared<wolkabout::ActuatorStatus>("VAL", "REF2", wolkabout::ActuatorStatus::State::BUSY);

    const std::shared_ptr<wolkabout::ActuatorStatus> status3 =
      std::make_shared<wolkabout::ActuatorStatus>("VAL", "REF", wolkabout::ActuatorStatus::State::ERROR);

    bool removeCalledForKey1 = false;
    bool removeCalledForKey2 = false;
    bool removeCalledForKey3 = false;

    EXPECT_CALL(
      *dataProtocol,
      makeMessageProxy(testing::_,
                       testing::Matcher<const std::vector<std::shared_ptr<wolkabout::ActuatorStatus>>&>(testing::_)))
      .Times(3)
      .WillRepeatedly(testing::InvokeWithoutArgs([&] { return new wolkabout::Message("", ""); }));

    EXPECT_CALL(*persistence, getActuatorStatusesKeys())
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs([&] {
          std::vector<std::string> keys;
          if (!removeCalledForKey1)
              keys.push_back(key1);
          if (!removeCalledForKey2)
              keys.push_back(key2);
          if (!removeCalledForKey3)
              keys.push_back(key3);
          return keys;
      }));

    EXPECT_CALL(*persistence, removeActuatorStatus(key1))
      .Times(1)
      .WillOnce(testing::Assign(&removeCalledForKey1, true));
    EXPECT_CALL(*persistence, removeActuatorStatus(key2))
      .Times(1)
      .WillOnce(testing::Assign(&removeCalledForKey2, true));
    EXPECT_CALL(*persistence, removeActuatorStatus(key3))
      .Times(1)
      .WillOnce(testing::Assign(&removeCalledForKey3, true));

    EXPECT_CALL(*persistence, getActuatorStatus(key1))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs([&] { return removeCalledForKey1 ? nullptr : status1; }));

    EXPECT_CALL(*persistence, getActuatorStatus(key2))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs([&] { return removeCalledForKey2 ? nullptr : status2; }));

    EXPECT_CALL(*persistence, getActuatorStatus(key3))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs([&] { return removeCalledForKey3 ? nullptr : status3; }));

    // When
    dataService->publishActuatorStatuses();

    ASSERT_EQ(connectivityService->getMessages().size(), 3);
}

TEST_F(
  DataService,
  Given_PersistedActuatorStatus_When_PublishActuatorStatusForDeviceKeyIsCalled_Then_ActuatorStatusArePublishedAndPersistenceIsEmptied)
{
    // Given
    const auto key1 = "KEY1+REF1";
    const auto key2 = "KEY1+REF2";
    const auto key3 = "KEY2+REF";

    const std::shared_ptr<wolkabout::ActuatorStatus> status1 =
      std::make_shared<wolkabout::ActuatorStatus>("VAL", "REF1", wolkabout::ActuatorStatus::State::READY);

    const std::shared_ptr<wolkabout::ActuatorStatus> status2 =
      std::make_shared<wolkabout::ActuatorStatus>("VAL", "REF2", wolkabout::ActuatorStatus::State::BUSY);

    const std::shared_ptr<wolkabout::ActuatorStatus> status3 =
      std::make_shared<wolkabout::ActuatorStatus>("VAL", "REF", wolkabout::ActuatorStatus::State::ERROR);

    bool removeCalledForKey1 = false;
    bool removeCalledForKey2 = false;
    // bool removeCalledForKey3 = false;

    EXPECT_CALL(
      *dataProtocol,
      makeMessageProxy(testing::_,
                       testing::Matcher<const std::vector<std::shared_ptr<wolkabout::ActuatorStatus>>&>(testing::_)))
      .Times(2)
      .WillRepeatedly(testing::InvokeWithoutArgs([&] { return new wolkabout::Message("", ""); }));

    EXPECT_CALL(*persistence, getActuatorStatusesKeys())
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs([&] {
          std::vector<std::string> keys;
          if (!removeCalledForKey1)
              keys.push_back(key1);
          if (!removeCalledForKey2)
              keys.push_back(key2);
          // if(!removeCalledForKey3)
          keys.push_back(key3);
          return keys;
      }));

    EXPECT_CALL(*persistence, removeActuatorStatus(key1))
      .Times(1)
      .WillOnce(testing::Assign(&removeCalledForKey1, true));
    ;

    EXPECT_CALL(*persistence, removeActuatorStatus(key2))
      .Times(1)
      .WillOnce(testing::Assign(&removeCalledForKey2, true));
    ;

    EXPECT_CALL(*persistence, removeActuatorStatus(key3)).Times(0);

    EXPECT_CALL(*persistence, getActuatorStatus(key1))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs([&] { return removeCalledForKey1 ? nullptr : status1; }));

    EXPECT_CALL(*persistence, getActuatorStatus(key2))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs([&] { return removeCalledForKey2 ? nullptr : status2; }));

    EXPECT_CALL(*persistence, getActuatorStatus(key3)).Times(0);

    // When
    dataService->publishActuatorStatuses("KEY1");

    ASSERT_EQ(connectivityService->getMessages().size(), 2);
}

TEST_F(
  DataService,
  Given_PersistedConfiguration_When_PublishConfigurationForAllIsCalled_Then_ConfigurationIsPublishedAndPersistenceIsEmptied)
{
    // Given
    const auto key1 = "KEY1";
    const auto key2 = "KEY2";
    const auto key3 = "KEY3";

    auto conf1 = std::make_shared<std::vector<wolkabout::ConfigurationItem>>(
      std::initializer_list<std::vector<wolkabout::ConfigurationItem>::value_type>{{{"V1"}, "R1"}, {{"V2"}, "R2"}});

    auto conf2 = std::make_shared<std::vector<wolkabout::ConfigurationItem>>(
      std::initializer_list<std::vector<wolkabout::ConfigurationItem>::value_type>{{{"V2"}, "R2"}});

    auto conf3 = std::make_shared<std::vector<wolkabout::ConfigurationItem>>(
      std::initializer_list<std::vector<wolkabout::ConfigurationItem>::value_type>{
        {{"V1"}, "R1"}, {{"V2"}, "R2"}, {{"V3"}, "R3"}});

    bool removeCalledForKey1 = false;
    bool removeCalledForKey2 = false;
    bool removeCalledForKey3 = false;

    EXPECT_CALL(
      *dataProtocol,
      makeMessageProxy(testing::_, testing::Matcher<const std::vector<wolkabout::ConfigurationItem>&>(testing::_),
                       testing::_))
      .Times(3)
      .WillRepeatedly(testing::InvokeWithoutArgs([&] { return new wolkabout::Message("", ""); }));

    EXPECT_CALL(*persistence, getConfigurationKeys())
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs([&] {
          std::vector<std::string> keys;
          if (!removeCalledForKey1)
              keys.push_back(key1);
          if (!removeCalledForKey2)
              keys.push_back(key2);
          if (!removeCalledForKey3)
              keys.push_back(key3);
          return keys;
      }));

    EXPECT_CALL(*persistence, removeConfiguration(key1)).Times(1).WillOnce(testing::Assign(&removeCalledForKey1, true));
    EXPECT_CALL(*persistence, removeConfiguration(key2)).Times(1).WillOnce(testing::Assign(&removeCalledForKey2, true));
    EXPECT_CALL(*persistence, removeConfiguration(key3)).Times(1).WillOnce(testing::Assign(&removeCalledForKey3, true));

    EXPECT_CALL(*persistence, getConfiguration(key1))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs([&] { return removeCalledForKey1 ? nullptr : conf1; }));

    EXPECT_CALL(*persistence, getConfiguration(key2))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs([&] { return removeCalledForKey2 ? nullptr : conf2; }));

    EXPECT_CALL(*persistence, getConfiguration(key3))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs([&] { return removeCalledForKey3 ? nullptr : conf3; }));

    // When
    dataService->publishConfiguration();

    ASSERT_EQ(connectivityService->getMessages().size(), 3);
}

TEST_F(
  DataService,
  Given_PersistedConfiguration_When_PublishConfigurationForDeviceKeyIsCalled_Then_ConfigurationIsPublishedAndPersistenceIsEmptied)
{
    // Given
    const auto key1 = "KEY1";
    const auto key2 = "KEY2";
    const auto key3 = "KEY3";

    auto conf1 = std::make_shared<std::vector<wolkabout::ConfigurationItem>>(
      std::initializer_list<std::vector<wolkabout::ConfigurationItem>::value_type>{{{"V1"}, "R1"}, {{"V2"}, "R2"}});

    auto conf2 = std::make_shared<std::vector<wolkabout::ConfigurationItem>>(
      std::initializer_list<std::vector<wolkabout::ConfigurationItem>::value_type>{{{"V2"}, "R2"}});

    auto conf3 = std::make_shared<std::vector<wolkabout::ConfigurationItem>>(
      std::initializer_list<std::vector<wolkabout::ConfigurationItem>::value_type>{
        {{"V1"}, "R1"}, {{"V2"}, "R2"}, {{"V3"}, "R3"}});

    // bool removeCalledForKey1 = false;
    // bool removeCalledForKey2 = false;
    bool removeCalledForKey3 = false;

    EXPECT_CALL(
      *dataProtocol,
      makeMessageProxy(testing::_, testing::Matcher<const std::vector<wolkabout::ConfigurationItem>&>(testing::_),
                       testing::_))
      .Times(1)
      .WillRepeatedly(testing::InvokeWithoutArgs([&] { return new wolkabout::Message("", ""); }));

    ON_CALL(*persistence, getConfigurationKeys()).WillByDefault(testing::InvokeWithoutArgs([&] {
        std::vector<std::string> keys;
        // if(!removeCalledForKey1)
        keys.push_back(key1);
        // if(!removeCalledForKey2)
        keys.push_back(key2);
        if (!removeCalledForKey3)
            keys.push_back(key3);
        return keys;
    }));

    EXPECT_CALL(*persistence, removeConfiguration(key1)).Times(0);
    EXPECT_CALL(*persistence, removeConfiguration(key2)).Times(0);
    EXPECT_CALL(*persistence, removeConfiguration(key3)).Times(1).WillOnce(testing::Assign(&removeCalledForKey3, true));

    EXPECT_CALL(*persistence, getConfiguration(key1)).Times(0);

    EXPECT_CALL(*persistence, getConfiguration(key2)).Times(0);

    EXPECT_CALL(*persistence, getConfiguration(key3))
      .Times(testing::AtLeast(1))
      .WillRepeatedly(testing::InvokeWithoutArgs([&] { return removeCalledForKey3 ? nullptr : conf3; }));

    // When
    dataService->publishConfiguration(key3);

    ASSERT_EQ(connectivityService->getMessages().size(), 1);
}
