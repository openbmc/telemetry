#pragma once

#include "types/duration_types.hpp"
#include "types/trigger_types.hpp"

#include <cstdint>
#include <string>

namespace interfaces
{

class TriggerAction
{
  public:
    virtual ~TriggerAction() = default;

    virtual void commit(const std::string& triggerId,
                        ThresholdName thresholdName,
                        const std::string& sensorId, Milliseconds timestamp,
                        TriggerValue value) = 0;
};
} // namespace interfaces
