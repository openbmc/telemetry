#pragma once

#include "interfaces/sensor.hpp"

#include <cstdint>

namespace interfaces
{

class TriggerAction
{
  public:
    virtual ~TriggerAction() = default;

    virtual void commit(const Sensor::Id& id, uint64_t timestamp,
                        double value) = 0;
};
} // namespace interfaces
