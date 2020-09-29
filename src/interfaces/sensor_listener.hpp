#pragma once

#include <optional>

namespace interfaces
{

class Sensor;

class SensorListener
{
  public:
    virtual ~SensorListener() = default;

    virtual void sensorUpdated(interfaces::Sensor&) = 0;
    virtual void sensorUpdated(interfaces::Sensor&, double) = 0;
};

} // namespace interfaces
