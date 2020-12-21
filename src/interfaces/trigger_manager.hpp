#pragma once

#include "interfaces/trigger.hpp"

namespace interfaces
{

class TriggerManager
{
  public:
    virtual ~TriggerManager() = default;

    virtual void removeTrigger(const Trigger* trigger) = 0;
};

} // namespace interfaces
