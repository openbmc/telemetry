#pragma once

#include "interfaces/sensor_listener.hpp"
#include "metric_value.hpp"
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
    virtual std::vector<MetricValue> getReadings() const = 0;
    virtual LabeledMetricParameters dumpConfiguration() const = 0;
    virtual uint64_t sensorCount() const = 0;
    virtual void registerForUpdates(interfaces::SensorListener& listener) = 0;
    virtual void
        unregisterFromUpdates(interfaces::SensorListener& listener) = 0;
};

} // namespace interfaces
