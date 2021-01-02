#pragma once

#include "interfaces/trigger.hpp"
#include "interfaces/trigger_manager.hpp"
#include "interfaces/types.hpp"

#include <boost/asio/spawn.hpp>

#include <memory>

namespace interfaces
{

class TriggerFactory
{
  public:
    virtual ~TriggerFactory() = default;

    virtual std::unique_ptr<interfaces::Trigger>
        make(boost::asio::yield_context& yield, const std::string& name,
             const bool isDiscrete, const bool logToJournal,
             const bool logToRedfish, const bool updateReport,
             const std::vector<sdbusplus::message::object_path>& sensors,
             const std::vector<std::string>& reportNames,
             const TriggerThresholdParams& thresholds,
             TriggerManager& triggerManager) const = 0;
};

} // namespace interfaces
