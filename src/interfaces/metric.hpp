#pragma once

#include "metric_value.hpp"
#include "types/types.hpp"

#include <nlohmann/json.hpp>

#include <vector>

namespace interfaces
{

class Metric
{
  public:
    virtual ~Metric() = default;

    virtual void initialize() = 0;
    virtual const std::vector<MetricValue>& getReadings() const = 0;
    virtual LabeledMetricParameters dumpConfiguration() const = 0;
};

} // namespace interfaces
