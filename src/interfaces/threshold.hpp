#pragma once

#include "interfaces/sensor.hpp"
#include "types/trigger_types.hpp"

namespace interfaces
{

class Threshold
{
  public:
    virtual ~Threshold() = default;

    virtual void initialize() = 0;
    virtual LabeledThresholdParam getThresholdParam() const = 0;
    virtual void updateSensors(Sensors newSensors) = 0;
};

} // namespace interfaces
