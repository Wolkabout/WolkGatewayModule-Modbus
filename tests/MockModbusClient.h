//
// Created by nvuletic on 11/4/19.
//

#ifndef WOLKGATEWAYMODBUSMODULE_MOCKMODBUSCLIENT_H
#define WOLKGATEWAYMODBUSMODULE_MOCKMODBUSCLIENT_H

#include "modbus/ModbusClient.h"
#include <gmock/gmock.h>
#include <gtest/internal/gtest-port.h>

class MockModbusClient : public wolkabout::ModbusClient
{
public:
    MockModbusClient(std::chrono::milliseconds responseTimeout) : ModbusClient(responseTimeout) { m_connected = false; }
    ~MockModbusClient() override {}

    bool createContext() override {}
    bool destroyContext() override {}

    bool connect() { m_connected = true; }
    bool disconnect() { m_connected = false; }
    bool isConnected() { return m_connected; }

    MOCK_METHOD3(writeHoldingRegister, bool(int, int, signed short));
    MOCK_METHOD3(writeHoldingRegister, bool(int, int, unsigned short));
    MOCK_METHOD3(writeHoldingRegister, bool(int, int, float));
    MOCK_METHOD3(writeHoldingRegisters, bool(int, int, std::vector<signed short>&));
    MOCK_METHOD3(writeHoldingRegisters, bool(int, int, std::vector<unsigned short>&));
    MOCK_METHOD3(writeHoldingRegisters, bool(int, int, std::vector<float>&));

    MOCK_METHOD3(writeCoil, bool(int, int, bool));

    MOCK_METHOD4(readInputRegisters, bool(int, int, int, std::vector<signed short>&));
    MOCK_METHOD4(readInputRegisters, bool(int, int, int, std::vector<unsigned short>&));
    MOCK_METHOD4(readInputRegisters, bool(int, int, int, std::vector<float>&));

    MOCK_METHOD4(readInputContacts, bool(int, int, int, std::vector<bool>&));

    MOCK_METHOD3(readHoldingRegister, bool(int, int, signed short&));
    MOCK_METHOD3(readHoldingRegister, bool(int, int, unsigned short&));
    MOCK_METHOD3(readHoldingRegister, bool(int, int, float&));
    MOCK_METHOD4(readHoldingRegisters, bool(int, int, int, std::vector<signed short>&));
    MOCK_METHOD4(readHoldingRegisters, bool(int, int, int, std::vector<unsigned short>&));
    MOCK_METHOD4(readHoldingRegisters, bool(int, int, int, std::vector<float>&));

    MOCK_METHOD3(readCoil, bool(int, int, bool&));
    MOCK_METHOD4(readCoils, bool(int, int, int, std::vector<bool>&));

private:
    bool m_connected;

    GTEST_DISALLOW_COPY_AND_ASSIGN_(MockModbusClient);
};

#endif    // WOLKGATEWAYMODBUSMODULE_MOCKMODBUSCLIENT_H
