#pragma once

#include "interfaces/json_storage.hpp"
#include "types/trigger_types.hpp"

namespace utils
{

class ToLabeledThresholdParamConversion
{
  public:
    LabeledTriggerThresholdParams operator()(const std::monostate& arg) const;
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

TriggerThresholdParams
    fromLabeledThresholdParam(const std::vector<LabeledThresholdParam>& params);

nlohmann::json labeledThresholdParamsToJson(
    const LabeledTriggerThresholdParams& labeledThresholdParams);

template <typename T>
struct is_variant : std::false_type
{};

template <typename... Args>
struct is_variant<std::variant<Args...>> : std::true_type
{};

template <typename T>
inline constexpr bool is_variant_v = is_variant<T>::value;

template <typename AlternativeT, typename VariantT>
    requires is_variant_v<VariantT> bool
isFirstElementOfType(const std::vector<VariantT>& collection)
{
    if (collection.empty())
    {
        return false;
    }
    return std::holds_alternative<AlternativeT>(*collection.begin());
}

double stodStrict(const std::string& str);

} // namespace utils
