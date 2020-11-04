#pragma once

#include "interfaces/types.hpp"
#include "metric_value.hpp"

namespace interfaces
{

class Metric
{
  public:
    virtual ~Metric() = default;

    virtual void initialize() = 0;
    virtual const std::vector<MetricValue>& getReadings() const = 0;
};

} // namespace interfaces
