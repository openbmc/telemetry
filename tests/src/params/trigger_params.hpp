#pragma once

#include "types/milliseconds.hpp"
#include "types/trigger_types.hpp"

#include <sdbusplus/message.hpp>

#include <chrono>
#include <utility>

class TriggerParams
{
  public:
    TriggerParams& id(std::string val)
    {
        idProperty = std::move(val);
        return *this;
    }

    const std::string& id() const
    {
        return idProperty;
    }

    TriggerParams& name(std::string val)
    {
        nameProperty = std::move(val);
        return *this;
    }

    const std::string& name() const
    {
        return nameProperty;
    }

    TriggerParams& triggerActions(const std::vector<TriggerAction>& val)
    {
        triggerActionsProperty = val;
        return *this;
    }

    const std::vector<TriggerAction>& triggerActions() const
    {
        return triggerActionsProperty;
    }

    const std::vector<LabeledSensorInfo>& sensors() const
    {
        return labeledSensorsProperty;
    }

    const std::vector<std::string>& reportIds() const
    {
        return reportIdsProperty;
    }

    TriggerParams& thresholdParams(LabeledTriggerThresholdParams val)
    {
        labeledThresholdsProperty = std::move(val);
        return *this;
    }

    const LabeledTriggerThresholdParams& thresholdParams() const
    {
        return labeledThresholdsProperty;
    }

  private:
    std::string idProperty = "Trigger1";
    std::string nameProperty = "My Numeric Trigger";
    std::vector<TriggerAction> triggerActionsProperty = {
        TriggerAction::UpdateReport};
    std::vector<LabeledSensorInfo> labeledSensorsProperty = {
        {"service1", "/xyz/openbmc_project/sensors/temperature/BMC_Temp",
         "metadata1"}};
    std::vector<std::string> reportIdsProperty = {"Report1"};
    LabeledTriggerThresholdParams labeledThresholdsProperty =
        std::vector<numeric::LabeledThresholdParam>{
            numeric::LabeledThresholdParam{numeric::Type::lowerCritical,
                                           Milliseconds(10).count(),
                                           numeric::Direction::decreasing, 0.5},
            numeric::LabeledThresholdParam{
                numeric::Type::upperCritical, Milliseconds(10).count(),
                numeric::Direction::increasing, 90.2}};
};
