#pragma once

#include "interfaces/metric.hpp"

#include <gmock/gmock.h>

class MetricMock : public interfaces::Metric
{
  public:
    MetricMock()
    {
        using namespace testing;

        ON_CALL(*this, getUpdatedReadings())
            .WillByDefault(ReturnRefOfCopy(std::vector<MetricValue>()));
        ON_CALL(*this, sensorCount).WillByDefault(InvokeWithoutArgs([this] {
            return getUpdatedReadings().size();
        }));
    }

    MOCK_METHOD(void, initialize, (), (override));
    MOCK_METHOD(void, deinitialize, (), (override));
    MOCK_METHOD(const std::vector<MetricValue>&, getUpdatedReadings, (),
                (override));
    MOCK_METHOD(LabeledMetricParameters, dumpConfiguration, (),
                (const, override));
    MOCK_METHOD(uint64_t, sensorCount, (), (const, override));
    MOCK_METHOD(void, registerForUpdates, (interfaces::MetricListener&),
                (override));
    MOCK_METHOD(void, unregisterFromUpdates, (interfaces::MetricListener&),
                (override));
    MOCK_METHOD(void, updateReadings, (Milliseconds), (override));
    MOCK_METHOD(bool, isTimerRequired, (), (const, override));
};
