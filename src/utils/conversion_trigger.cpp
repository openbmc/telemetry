#include "utils/conversion_trigger.hpp"

#include "utils/transform.hpp"

namespace utils
{
namespace ts = utils::tstring;

LabeledTriggerThresholdParams ToLabeledThresholdParamConversion::operator()(
    const std::vector<numeric::ThresholdParam>& arg) const
{
    return utils::transform(arg, [](const auto& thresholdParam) {
        const auto& [type, dwellTime, direction, thresholdValue] =
            thresholdParam;
        return numeric::LabeledThresholdParam(
            numeric::stringToType(type), dwellTime,
            numeric::stringToDirection(direction), thresholdValue);
    });
}

LabeledTriggerThresholdParams ToLabeledThresholdParamConversion::operator()(
    const std::vector<discrete::ThresholdParam>& arg) const
{
    return utils::transform(arg, [](const auto& thresholdParam) {
        const auto& [userId, severity, thresholdValue, dwellTime] =
            thresholdParam;
        return discrete::LabeledThresholdParam(
            userId, discrete::stringToSeverity(severity), thresholdValue,
            dwellTime);
    });
}

TriggerThresholdParams FromLabeledThresholdParamConversion::operator()(
    const std::vector<numeric::LabeledThresholdParam>& arg) const
{
    return utils::transform(
        arg, [](const numeric::LabeledThresholdParam& labeledThresholdParam) {
            return numeric::ThresholdParam(
                numeric::typeToString(
                    labeledThresholdParam.at_label<ts::Type>()),
                labeledThresholdParam.at_label<ts::DwellTime>(),
                numeric::directionToString(
                    labeledThresholdParam.at_label<ts::Direction>()),
                labeledThresholdParam.at_label<ts::ThresholdValue>());
        });
}

TriggerThresholdParams FromLabeledThresholdParamConversion::operator()(
    const std::vector<discrete::LabeledThresholdParam>& arg) const
{
    return utils::transform(
        arg, [](const discrete::LabeledThresholdParam& labeledThresholdParam) {
            return discrete::ThresholdParam(
                labeledThresholdParam.at_label<ts::UserId>(),
                discrete::severityToString(
                    labeledThresholdParam.at_label<ts::Severity>()),
                labeledThresholdParam.at_label<ts::ThresholdValue>(),
                labeledThresholdParam.at_label<ts::DwellTime>());
        });
}

SensorsInfo fromLabeledSensorsInfo(const std::vector<LabeledSensorInfo>& infos)
{
    return utils::transform(infos, [](const LabeledSensorInfo& val) {
        return SensorsInfo::value_type(
            sdbusplus::message::object_path(val.at_label<ts::SensorPath>()),
            val.at_label<ts::SensorMetadata>());
    });
}

nlohmann::json labeledThresholdParamsToJson(
    const LabeledTriggerThresholdParams& labeledThresholdParams)
{
    return std::visit([](const auto& lt) { return nlohmann::json(lt); },
                      labeledThresholdParams);
}

} // namespace utils
