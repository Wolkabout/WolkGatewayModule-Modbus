//
// Created by nvuletic on 11/4/19.
//

#define private public
#define protected public
#include "MockModbusClient.h"
#include "modbus/ModbusBridge.h"

#undef private
#undef protected

#include <gtest/gtest.h>

class BridgeTest : public ::testing::Test
{
public:
    void SetUp() override {}

private:
    MockModbusClient modbusClient;
    wolkabout::ModbusBridge modbusBridge;
};

TEST(BridgeTest, SimpleTest)
{
    std::cerr << "Hello from test!" << std::endl;
}