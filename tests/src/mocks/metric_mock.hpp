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
            .WillByDefault(ReturnRefOfCopy(std::vector<MetricValue>()));
    }

    MOCK_METHOD(void, initialize, (), (override));
    MOCK_METHOD(const std::vector<MetricValue>&, getReadings, (),
                (const, override));
    MOCK_METHOD(nlohmann::json, to_json, (), (const, override));
};
