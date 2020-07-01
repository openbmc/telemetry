#include "core/report_mgr_ut.hpp"

#include "gtest/gtest.h"

// TODO: Find different solution
core::SensorCache::AllSensors core::SensorCache::sensors;

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    return ret;
}
