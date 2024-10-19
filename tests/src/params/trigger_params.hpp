#pragma once

#include "report.hpp"
#include "types/duration_types.hpp"
#include "types/trigger_types.hpp"
#include "utils/dbus_path_utils.hpp"
#include "utils/transform.hpp"

#include <sdbusplus/message.hpp>

#include <chrono>
#include <utility>

using sdbusplus::message::object_path;

class TriggerParams
{
  public:
    TriggerParams()
    {
        reportsProperty = utils::transform(reportIdsProperty,
                                           [](const auto& id) {
            return utils::pathAppend(utils::constants::reportDirPath, id);
        });
    }

    TriggerParams& id(std::string_view val)
    {
        idProperty = val;
        return *this;
    }

    const std::string& id() const
    {
        return idProperty;
    }

    TriggerParams& name(std::string_view val)
    {
        nameProperty = val;
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

    TriggerParams& reportIds(std::vector<std::string> val)
    {
        reportIdsProperty = std::move(val);
        reportsProperty = utils::transform<std::vector>(reportIdsProperty,
                                                        [](const auto& id) {
            return utils::pathAppend(utils::constants::reportDirPath, id);
        });
        return *this;
    }

    const std::vector<object_path>& reports() const
    {
        return reportsProperty;
    }

    TriggerParams& reports(std::vector<object_path> val)
    {
        reportsProperty = std::move(val);
        return *this;
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

    const std::vector<numeric::LabeledThresholdParam>
        numericThresholdParams() const
    {
        const auto* num = std::get_if<0>(&labeledThresholdsProperty);
        if (num == nullptr)
        {
            return {};
        }
        return *num;
    }

    const std::vector<discrete::LabeledThresholdParam>
        discreteThresholdParams() const
    {
        const auto* num = std::get_if<1>(&labeledThresholdsProperty);
        if (num == nullptr)
        {
            return {};
        }
        return *num;
    }

  private:
    std::string idProperty = "Trigger1";
    std::string nameProperty = "My Numeric Trigger";
    std::vector<TriggerAction> triggerActionsProperty = {
        TriggerAction::UpdateReport};
    std::vector<LabeledSensorInfo> labeledSensorsProperty = {
        {"service1", "/xyz/openbmc_project/sensors/temperature/BMC_Temp",
         "metadata1"}};
    std::vector<std::string> reportIdsProperty = {"Report1",
                                                  "Prefixed/Report2"};
    std::vector<object_path> reportsProperty;
    LabeledTriggerThresholdParams labeledThresholdsProperty =
        std::vector<numeric::LabeledThresholdParam>{
            numeric::LabeledThresholdParam{numeric::Type::lowerCritical,
                                           Milliseconds(10).count(),
                                           numeric::Direction::decreasing, 0.5},
            numeric::LabeledThresholdParam{
                numeric::Type::upperCritical, Milliseconds(10).count(),
                numeric::Direction::increasing, 90.2}};
};
