#pragma once

#include "interfaces/json_storage.hpp"
#include "interfaces/trigger_types.hpp"

namespace utils
{

class ToLabeledThresholdParamConversion
{
  public:
    LabeledTriggerThresholdParams
        operator()(const std::vector<numeric::ThresholdParam>& arg) const;
    LabeledTriggerThresholdParams
        operator()(const std::vector<discrete::ThresholdParam>& arg) const;
};

class FromLabeledThresholdParamConversion
{
  public:
    TriggerThresholdParams operator()(
        const std::vector<numeric::LabeledThresholdParam>& arg) const;
    TriggerThresholdParams operator()(
        const std::vector<discrete::LabeledThresholdParam>& arg) const;
};

SensorsInfo fromLabeledSensorsInfo(const std::vector<LabeledSensorInfo>& infos);

nlohmann::json labeledThresholdParamsToJson(
    const LabeledTriggerThresholdParams& labeledThresholdParams);

} // namespace utils
