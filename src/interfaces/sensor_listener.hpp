#pragma once

#include "types/duration_types.hpp"

#include <cstdint>
#include <optional>

namespace interfaces
{

class Sensor;

class SensorListener
{
  public:
    virtual ~SensorListener() = default;

    virtual void sensorUpdated(interfaces::Sensor&, Milliseconds) = 0;
    virtual void sensorUpdated(interfaces::Sensor&, Milliseconds, double) = 0;
};

} // namespace interfaces
