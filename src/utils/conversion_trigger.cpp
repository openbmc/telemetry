#include "utils/conversion_trigger.hpp"

#include "utils/transform.hpp"
#include "utils/tstring.hpp"

#include <sdbusplus/exception.hpp>

namespace utils
{
namespace ts = utils::tstring;

LabeledTriggerThresholdParams ToLabeledThresholdParamConversion::operator()(
    const std::monostate& arg) const
{
    throw sdbusplus::exception::SdBusError(
        static_cast<int>(std::errc::invalid_argument),
        "Provided threshold parameter is invalid");
}

LabeledTriggerThresholdParams ToLabeledThresholdParamConversion::operator()(
    const std::vector<numeric::ThresholdParam>& arg) const
{
    return utils::transform(arg, [](const auto& thresholdParam) {
        const auto& [type, dwellTime, direction, thresholdValue] =
            thresholdParam;
        return numeric::LabeledThresholdParam(numeric::toType(type), dwellTime,
                                              numeric::toDirection(direction),
                                              thresholdValue);
    });
}

LabeledTriggerThresholdParams ToLabeledThresholdParamConversion::operator()(
    const std::vector<discrete::ThresholdParam>& arg) const
{
    return utils::transform(arg, [](const auto& thresholdParam) {
        const auto& [userId, severity, dwellTime, thresholdValue] =
            thresholdParam;
        return discrete::LabeledThresholdParam(
            userId, utils::toSeverity(severity), dwellTime, thresholdValue);
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
                utils::enumToString(
                    labeledThresholdParam.at_label<ts::Severity>()),
                labeledThresholdParam.at_label<ts::DwellTime>(),
                labeledThresholdParam.at_label<ts::ThresholdValue>());
        });
}

SensorsInfo fromLabeledSensorsInfo(const std::vector<LabeledSensorInfo>& infos)
{
    return utils::transform(infos, [](const LabeledSensorInfo& val) {
        return SensorsInfo::value_type(
            sdbusplus::message::object_path(val.at_label<ts::Path>()),
            val.at_label<ts::Metadata>());
    });
}

TriggerThresholdParams
    fromLabeledThresholdParam(const std::vector<LabeledThresholdParam>& params)
{
    namespace ts = utils::tstring;
    if (params.empty())
    {
        return std::vector<numeric::ThresholdParam>();
    }

    if (isFirstElementOfType<std::monostate>(params))
    {
        return std::vector<discrete::ThresholdParam>();
    }

    if (isFirstElementOfType<discrete::LabeledThresholdParam>(params))
    {
        return utils::transform(params, [](const auto& param) {
            const discrete::LabeledThresholdParam* paramUnpacked =
                std::get_if<discrete::LabeledThresholdParam>(&param);
            if (!paramUnpacked)
            {
                throw std::runtime_error(
                    "Mixing threshold types is not allowed");
            }
            return discrete::ThresholdParam(
                paramUnpacked->at_label<ts::UserId>(),
                utils::enumToString(paramUnpacked->at_label<ts::Severity>()),
                paramUnpacked->at_label<ts::DwellTime>(),
                paramUnpacked->at_label<ts::ThresholdValue>());
        });
    }

    if (isFirstElementOfType<numeric::LabeledThresholdParam>(params))
    {
        return utils::transform(params, [](const auto& param) {
            const numeric::LabeledThresholdParam* paramUnpacked =
                std::get_if<numeric::LabeledThresholdParam>(&param);
            if (!paramUnpacked)
            {
                throw std::runtime_error(
                    "Mixing threshold types is not allowed");
            }
            return numeric::ThresholdParam(
                numeric::typeToString(paramUnpacked->at_label<ts::Type>()),
                paramUnpacked->at_label<ts::DwellTime>(),
                numeric::directionToString(
                    paramUnpacked->at_label<ts::Direction>()),
                paramUnpacked->at_label<ts::ThresholdValue>());
        });
    }

    throw std::runtime_error("Incorrect threshold params");
}

nlohmann::json labeledThresholdParamsToJson(
    const LabeledTriggerThresholdParams& labeledThresholdParams)
{
    return std::visit([](const auto& lt) { return nlohmann::json(lt); },
                      labeledThresholdParams);
}

double stodStrict(const std::string& str)
{
    size_t pos = 0;
    double result = std::stod(str, &pos);
    if (pos < str.length())
    {
        throw std::invalid_argument(
            "non-numeric characters at the end of string");
    }
    return result;
}

} // namespace utils
