#pragma once

#include <optional>

namespace interfaces
{

class Sensor;

class SensorListener
{
  public:
    virtual ~SensorListener() = default;

    virtual void sensorUpdated(interfaces::Sensor&, uint64_t) = 0;
    virtual void sensorUpdated(interfaces::Sensor&, uint64_t, double) = 0;
};

} // namespace interfaces
