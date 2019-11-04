//
// Created by nvuletic on 11/4/19.
//

#define private public
#define protected public
#include "modbus/ModbusBridge.h"
#undef private
#undef protected

#include <gtest/gtest.h>

class BridgeTest : public ::testing::Test
{
public:
    void SetUp() override { std::cerr << "Hello from Test SetUp!" << std::endl; }
};

TEST(BridgeTest, SimpleTest)
{
    std::cerr << "Hello from test!" << std::endl;
}