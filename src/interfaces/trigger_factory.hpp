#pragma once

#include "interfaces/json_storage.hpp"
#include "interfaces/trigger.hpp"
#include "interfaces/trigger_manager.hpp"
#include "types/trigger_types.hpp"

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
        const std::string& name, const std::vector<std::string>& triggerActions,
        const std::vector<std::string>& reportNames,
        interfaces::TriggerManager& triggerManager,
        interfaces::JsonStorage& triggerStorage,
        const LabeledTriggerThresholdParams& labeledThresholdParams,
        const std::vector<LabeledSensorInfo>& labeledSensorsInfo) const = 0;

    virtual std::vector<LabeledSensorInfo>
        getLabeledSensorsInfo(boost::asio::yield_context& yield,
                              const SensorsInfo& sensorsInfo) const = 0;
};

} // namespace interfaces
