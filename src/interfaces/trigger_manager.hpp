#pragma once

#include "interfaces/trigger.hpp"

namespace interfaces
{

class TriggerManager
{
  public:
    virtual ~TriggerManager() = default;

    virtual void removeTrigger(const Trigger* trigger) = 0;

    virtual std::vector<std::string>
        getTriggerIdsForReport(const std::string& reportId) const = 0;
};

} // namespace interfaces
