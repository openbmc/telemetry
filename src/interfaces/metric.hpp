#pragma once

#include "metric_value.hpp"

#include <vector>

namespace interfaces
{

class Metric
{
  public:
    virtual ~Metric() = default;

    virtual const std::vector<MetricValue>& getReadings() const = 0;
};

} // namespace interfaces
