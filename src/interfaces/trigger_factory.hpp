#pragma once

#include "interfaces/json_storage.hpp"
#include "interfaces/sensor.hpp"
#include "interfaces/threshold.hpp"
#include "interfaces/trigger.hpp"
#include "interfaces/trigger_manager.hpp"
#include "sensor.hpp"
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
        const std::string& id, const std::string& name,
        const std::vector<std::string>& triggerActions,
        const std::vector<std::string>& reportIds,
        interfaces::TriggerManager& triggerManager,
        interfaces::JsonStorage& triggerStorage,
        const LabeledTriggerThresholdParams& labeledThresholdParams,
        const std::vector<LabeledSensorInfo>& labeledSensorsInfo) const = 0;

    virtual std::vector<LabeledSensorInfo>
        getLabeledSensorsInfo(boost::asio::yield_context& yield,
                              const SensorsInfo& sensorsInfo) const = 0;

    virtual std::vector<LabeledSensorInfo>
        getLabeledSensorsInfo(const SensorsInfo& sensorsInfo) const = 0;

    virtual void updateThresholds(
        std::vector<std::shared_ptr<interfaces::Threshold>>& currentThresholds,
        const std::vector<::TriggerAction>& triggerActions,
        const std::shared_ptr<std::vector<std::string>>& reportIds,
        const Sensors& sensors,
        const LabeledTriggerThresholdParams& newParams) const = 0;

    virtual void updateSensors(
        Sensors& currentSensors,
        const std::vector<LabeledSensorInfo>& labeledSensorsInfo) const = 0;
};

} // namespace interfaces
