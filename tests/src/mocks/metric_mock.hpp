#pragma once

#include "interfaces/metric.hpp"

#include <gmock/gmock.h>

class MetricMock : public interfaces::Metric
{
  public:
    MetricMock()
    {
        using namespace testing;

        ON_CALL(*this, getReadings())
            .WillByDefault(Return(std::vector<MetricValue>()));
    }

    MOCK_METHOD(void, initialize, (), (override));
    MOCK_METHOD(std::vector<MetricValue>, getReadings, (), (const, override));
    MOCK_METHOD(LabeledMetricParameters, dumpConfiguration, (),
                (const, override));

    uint64_t sensorCount() const override
    {
        return getReadings().size();
    }
};
