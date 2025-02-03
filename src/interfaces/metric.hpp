#pragma once

#include "interfaces/metric_listener.hpp"
#include "metric_value.hpp"
#include "types/duration_types.hpp"
#include "types/report_types.hpp"

#include <nlohmann/json.hpp>

#include <vector>

namespace interfaces
{

class Metric
{
  public:
    virtual ~Metric() = default;

    virtual void initialize() = 0;
    virtual void deinitialize() = 0;
    virtual const std::vector<MetricValue>& getUpdatedReadings() = 0;
    virtual LabeledMetricParameters dumpConfiguration() const = 0;
    virtual uint64_t metricCount() const = 0;
    virtual void registerForUpdates(interfaces::MetricListener& listener) = 0;
    virtual void unregisterFromUpdates(
        interfaces::MetricListener& listener) = 0;
    virtual void updateReadings(Milliseconds) = 0;
    virtual bool isTimerRequired() const = 0;
};

} // namespace interfaces
