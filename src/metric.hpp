#pragma once

#include "interfaces/metric.hpp"
#include "interfaces/sensor_listener.hpp"

class Metric : public interfaces::Metric, public interfaces::SensorListener
{
  public:
    const std::vector<MetricValue>& getReadings() const override
    {
        return readings;
    }

    void sensorUpdated(interfaces::Sensor&, uint64_t) override
    {}

    void sensorUpdated(interfaces::Sensor&, uint64_t, double value) override
    {}

  private:
    std::vector<MetricValue> readings;
};
