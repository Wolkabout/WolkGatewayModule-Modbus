#ifndef MOCKPERSISTANCE_H
#define MOCKPERSISTANCE_H

#include "model/ActuatorStatus.h"
#include "model/Alarm.h"
#include "model/SensorReading.h"
#include "persistence/Persistence.h"

#include <map>
#include <string>
#include <vector>

#include <gmock/gmock.h>

class MockPersistence : public wolkabout::Persistence
{
public:
    MockPersistence() {}
    virtual ~MockPersistence() {}

    MOCK_METHOD2(putSensorReading,
                 bool(const std::string& key, std::shared_ptr<wolkabout::SensorReading> sensorReading));

    MOCK_METHOD2(getSensorReadings, std::vector<std::shared_ptr<wolkabout::SensorReading>>(const std::string& key,
                                                                                           std::uint_fast64_t count));
    MOCK_METHOD2(removeSensorReadings, void(const std::string& key, std::uint_fast64_t count));

    MOCK_METHOD0(getSensorReadingsKeys, std::vector<std::string>());

    MOCK_METHOD2(putAlarm, bool(const std::string& key, std::shared_ptr<wolkabout::Alarm> alarm));

    MOCK_METHOD2(getAlarms,
                 std::vector<std::shared_ptr<wolkabout::Alarm>>(const std::string& key, std::uint_fast64_t count));
    MOCK_METHOD2(removeAlarms, void(const std::string& key, std::uint_fast64_t count));

    MOCK_METHOD0(getAlarmsKeys, std::vector<std::string>());

    MOCK_METHOD2(putActuatorStatus,
                 bool(const std::string& key, std::shared_ptr<wolkabout::ActuatorStatus> actuatorStatus));

    MOCK_METHOD1(getActuatorStatus, std::shared_ptr<wolkabout::ActuatorStatus>(const std::string& key));

    MOCK_METHOD1(removeActuatorStatus, void(const std::string& key));

    MOCK_METHOD0(getActuatorStatusesKeys, std::vector<std::string>());

    MOCK_METHOD2(putConfiguration, bool(const std::string& key,
                                        std::shared_ptr<std::vector<wolkabout::ConfigurationItem>> configuration));

    MOCK_METHOD1(getConfiguration, std::shared_ptr<std::vector<wolkabout::ConfigurationItem>>(const std::string& key));

    MOCK_METHOD1(removeConfiguration, void(const std::string& key));

    MOCK_METHOD0(getConfigurationKeys, std::vector<std::string>());

    MOCK_METHOD0(isEmpty, bool());

private:
    GTEST_DISALLOW_COPY_AND_ASSIGN_(MockPersistence);
};

#endif    // MOCKPERSISTANCE_H
