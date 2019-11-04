//
// Created by nvuletic on 11/4/19.
//

#define private public
#define protected public
#include "modbus/ModbusBridge.h"
#undef private
#undef protected

#include "MockModbusClient.h"
#include <gtest/gtest.h>

class BridgeTest : public ::testing::Test
{
public:
    void SetUp() override
    {
        const uint16_t clientTimeout = 250;
        const uint16_t readPeriod = 1000;
        std::vector<wolkabout::ModbusRegisterMapping> mappings;

        modbusClient =
          std::unique_ptr<MockModbusClient>(new MockModbusClient(std::chrono::milliseconds(clientTimeout)));
        modbusBridge = std::make_shared<wolkabout::ModbusBridge>(*modbusClient, std::move(mappings),
                                                                 std::chrono::milliseconds(readPeriod));
    }

    std::unique_ptr<MockModbusClient> modbusClient;
    std::shared_ptr<wolkabout::ModbusBridge> modbusBridge;
};

TEST_F(BridgeTest, SimpleTest)
{
    //    modbusBridge->onSensorReading([&](const std::string& reference, const std::string& value) {
    //
    //    });
}