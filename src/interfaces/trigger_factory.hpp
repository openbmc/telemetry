#pragma once

#include "interfaces/trigger.hpp"
#include "interfaces/trigger_manager.hpp"
#include "interfaces/trigger_types.hpp"

#include <boost/asio/spawn.hpp>
#include <sdbusplus/message/types.hpp>

#include <memory>
#include <utility>

namespace interfaces
{

class TriggerFactory
{
  public:
    virtual ~TriggerFactory() = default;

    virtual std::unique_ptr<interfaces::Trigger> make(
        boost::asio::yield_context& yield, const std::string& name,
        bool isDiscrete, bool logToJournal, bool logToRedfish,
        bool updateReport,
        const std::vector<
            std::pair<sdbusplus::message::object_path, std::string>>& sensors,
        const std::vector<std::string>& reportNames,
        const TriggerThresholdParams& thresholdParams,
        interfaces::TriggerManager& triggerManager) const = 0;
};

} // namespace interfaces
