#ifndef DATAPROTOCOL_H
#define DATAPROTOCOL_H

#include "protocol/Protocol.h"

#include <memory>
#include <vector>

namespace wolkabout
{
class Message;
class ActuatorSetCommand;
class ActuatorGetCommand;
class ActuatorStatus;
class Alarm;
class SensorReading;

class DataProtocol : public Protocol
{
public:
    virtual std::string extractReferenceFromChannel(const std::string& topic) const = 0;

    virtual bool isActuatorSetMessage(const std::string& channel) const = 0;
    virtual bool isActuatorGetMessage(const std::string& channel) const = 0;

    virtual std::unique_ptr<ActuatorGetCommand> makeActuatorGetCommand(std::shared_ptr<Message> message) const = 0;
    virtual std::unique_ptr<ActuatorSetCommand> makeActuatorSetCommand(std::shared_ptr<Message> message) const = 0;

    virtual std::shared_ptr<Message> makeMessage(const std::string& deviceKey,
                                                 std::vector<std::shared_ptr<SensorReading>> sensorReadings) const = 0;
    virtual std::shared_ptr<Message> makeMessage(const std::string& deviceKey,
                                                 std::vector<std::shared_ptr<Alarm>> alarms) const = 0;
    virtual std::shared_ptr<Message> makeMessage(
      const std::string& deviceKey, std::vector<std::shared_ptr<ActuatorStatus>> actuatorStatuses) const = 0;

    inline Type getType() const override final { return Protocol::Type::DATA; }
};
}    // namespace wolkabout

#endif    // DATAPROTOCOL_H
