#pragma once

#include "interfaces/metric_listener.hpp"

#include <gmock/gmock.h>

class MetricListenerMock : public interfaces::MetricListener
{
  public:
    MOCK_METHOD(void, metricUpdated, (), (override));
};
