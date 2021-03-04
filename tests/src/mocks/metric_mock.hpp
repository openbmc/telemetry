#pragma once

#include "interfaces/metric.hpp"

#include <gmock/gmock.h>

class MetricMock : public interfaces::Metric
{
  public:
    MetricMock()
    {
        using namespace testing;

        ON_CALL(*this, getReading()).WillByDefault(Return(MetricValue()));
    }

    MOCK_METHOD(void, initialize, (), (override));
    MOCK_METHOD(MetricValue, getReading, (), (const, override));
    MOCK_METHOD(LabeledMetricParameters, dumpConfiguration, (),
                (const, override));
};
