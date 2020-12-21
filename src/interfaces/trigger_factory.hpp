#pragma once

#include "interfaces/trigger.hpp"
#include "interfaces/trigger_manager.hpp"
#include "interfaces/types.hpp"

#include <memory>

namespace interfaces
{

class TriggerFactory
{
  public:
    virtual ~TriggerFactory() = default;

    virtual std::unique_ptr<interfaces::Trigger>
        make(const std::string& name, const bool isDiscrete,
             const bool logToJournal, const bool logToRedfish,
             const bool updateReport,
             const std::vector<sdbusplus::message::object_path>& sensors,
             const std::vector<std::string>& reportNames,
             const TriggerThresholds& thresholds,
             TriggerManager& triggerManager) const = 0;
};

} // namespace interfaces
