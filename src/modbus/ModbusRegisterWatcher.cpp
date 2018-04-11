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

#include "modbus/ModbusRegisterWatcher.h"

#include <string>

namespace wolkabout
{
ModbusRegisterWatcher::ModbusRegisterWatcher() : m_isInitialized(false) {}

bool ModbusRegisterWatcher::update(const std::string& newValue)
{
    bool isValueUpdated = m_value != newValue;
    m_value = newValue;

    bool isValueInitialized = m_isInitialized;
    if (!m_isInitialized)
    {
        m_isInitialized = true;
    }

    return !isValueInitialized || isValueUpdated;
}

const std::string& ModbusRegisterWatcher::getValue() const
{
    return m_value;
}

bool ModbusRegisterWatcher::update(signed short newRegisterValue)
{
    const auto newRegisterValueStr = std::to_string(newRegisterValue);
    return update(newRegisterValueStr);
}

bool ModbusRegisterWatcher::update(unsigned short newRegisterValue)
{
    const auto newRegisterValueStr = std::to_string(newRegisterValue);
    return update(newRegisterValueStr);
}

bool ModbusRegisterWatcher::update(float newRegisterValue)
{
    const auto newRegisterValueStr = std::to_string(newRegisterValue);
    return update(newRegisterValueStr);
}

bool ModbusRegisterWatcher::update(bool newRegisterValue)
{
    const auto newRegisterValueStr = std::to_string(newRegisterValue);
    return update(newRegisterValueStr);
}
}    // namespace wolkabout
