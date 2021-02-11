#pragma once

#include "types/types.hpp"
#include "metric_value.hpp"

#include <nlohmann/json.hpp>

#include <vector>

namespace interfaces
{

class Metric
{
  public:
    virtual ~Metric() = default;

    virtual void initialize() = 0;
    virtual const MetricValue& getReading() const = 0;
    virtual LabeledMetricParameters dumpConfiguration() const = 0;
};

} // namespace interfaces
