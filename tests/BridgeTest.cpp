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
        mappings.emplace_back(
          "TEST_SENSOR_BOOL", "TSB", "", 0, 1, 0, wolkabout::ModbusRegisterMapping::RegisterType::INPUT_CONTACT,
          wolkabout::ModbusRegisterMapping::DataType::BOOL, 1, wolkabout::ModbusRegisterMapping::MappingType::DEFAULT);
        mappings.emplace_back(
          "TEST_SENSOR_INT16", "TSI", "", 0, 65535, 0, wolkabout::ModbusRegisterMapping::RegisterType::INPUT_REGISTER,
          wolkabout::ModbusRegisterMapping::DataType::INT16, 1, wolkabout::ModbusRegisterMapping::MappingType::DEFAULT);

        MockModbusClient modbusClient((std::chrono::milliseconds(clientTimeout)));
        modbusBridge =
          std::make_shared<wolkabout::ModbusBridge>(modbusClient, mappings, std::chrono::milliseconds(readPeriod));
    }

public:
    std::shared_ptr<wolkabout::ModbusBridge> modbusBridge;
};

TEST_F(BridgeTest, SimpleTest) {}