#pragma once

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
    virtual const std::vector<MetricValue>& getReadings() const = 0;
    virtual nlohmann::json to_json() const = 0;
};

} // namespace interfaces
