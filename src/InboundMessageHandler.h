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

#ifndef INBOUNDMESSAGEHANDLER_H
#define INBOUNDMESSAGEHANDLER_H

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class Message;
class Protocol;

class MessageListener
{
public:
    virtual ~MessageListener() = default;
    virtual void messageReceived(std::shared_ptr<Message> message) = 0;
    virtual const Protocol& getProtocol() = 0;
};

class InboundMessageHandler
{
public:
    virtual ~InboundMessageHandler() = default;

    virtual void messageReceived(const std::string& channel, const std::string& message) = 0;

    virtual std::vector<std::string> getChannels() const = 0;

    virtual void addListener(std::weak_ptr<MessageListener> listener) = 0;
};
}    // namespace wolkabout

#endif    // INBOUNDMESSAGEHANDLER_H
