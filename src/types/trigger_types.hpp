#pragma once

#include "types/sensor_types.hpp"
#include "utils/conversion.hpp"
#include "utils/labeled_tuple.hpp"
#include "utils/tstring.hpp"

#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

enum class TriggerAction
{
    LogToJournal = 0,
    LogToRedfishEventLog,
    UpdateReport,
};

namespace details
{
constexpr std::array<std::pair<std::string_view, TriggerAction>, 3>
    convDataTriggerAction = {
        std::make_pair("LogToJournal", TriggerAction::LogToJournal),
        std::make_pair("LogToRedfishEventLog",
                       TriggerAction::LogToRedfishEventLog),
        std::make_pair("UpdateReport", TriggerAction::UpdateReport)};
}

inline TriggerAction toTriggerAction(const std::string& str)
{
    return utils::toEnum(details::convDataTriggerAction, str);
}

inline std::string actionToString(TriggerAction v)
{
    return std::string(utils::enumToString(details::convDataTriggerAction, v));
}

namespace discrete
{

enum class Severity
{
    ok = 0,
    warning,
    critical
};

namespace details
{
constexpr std::array<std::pair<std::string_view, Severity>, 3>
    convDataSeverity = {std::make_pair("OK", Severity::ok),
                        std::make_pair("Warning", Severity::warning),
                        std::make_pair("Critical", Severity::critical)};

} // namespace details

inline Severity toSeverity(const std::string& str)
{
    return utils::toEnum(details::convDataSeverity, str);
}

inline std::string severityToString(Severity v)
{
    return std::string(utils::enumToString(details::convDataSeverity, v));
}

using ThresholdParam =
    std::tuple<std::string, std::string, uint64_t, std::string>;

using LabeledThresholdParam = utils::LabeledTuple<
    std::tuple<std::string, Severity, uint64_t, std::string>,
    utils::tstring::UserId, utils::tstring::Severity, utils::tstring::DwellTime,
    utils::tstring::ThresholdValue>;
} // namespace discrete

namespace numeric
{

enum class Type
{
    lowerCritical = 0,
    lowerWarning,
    upperWarning,
    upperCritical
};

enum class Direction
{
    either = 0,
    decreasing,
    increasing
};

namespace details
{

constexpr std::array<std::pair<std::string_view, Type>, 4> convDataType = {
    std::make_pair("LowerCritical", Type::lowerCritical),
    std::make_pair("LowerWarning", Type::lowerWarning),
    std::make_pair("UpperWarning", Type::upperWarning),
    std::make_pair("UpperCritical", Type::upperCritical)};

constexpr std::array<std::pair<std::string_view, Direction>, 3>
    convDataDirection = {std::make_pair("Either", Direction::either),
                         std::make_pair("Decreasing", Direction::decreasing),
                         std::make_pair("Increasing", Direction::increasing)};

} // namespace details

inline Type toType(const std::string& str)
{
    return utils::toEnum(details::convDataType, str);
}

inline std::string typeToString(Type v)
{
    return std::string(utils::enumToString(details::convDataType, v));
}

inline Direction toDirection(const std::string& str)
{
    return utils::toEnum(details::convDataDirection, str);
}

inline std::string directionToString(Direction v)
{
    return std::string(utils::enumToString(details::convDataDirection, v));
}

using ThresholdParam = std::tuple<std::string, uint64_t, std::string, double>;

using LabeledThresholdParam =
    utils::LabeledTuple<std::tuple<Type, uint64_t, Direction, double>,
                        utils::tstring::Type, utils::tstring::DwellTime,
                        utils::tstring::Direction,
                        utils::tstring::ThresholdValue>;
} // namespace numeric

using TriggerThresholdParamsExt =
    std::variant<std::monostate, std::vector<numeric::ThresholdParam>,
                 std::vector<discrete::ThresholdParam>>;

using TriggerThresholdParams =
    std::variant<std::vector<numeric::ThresholdParam>,
                 std::vector<discrete::ThresholdParam>>;

using LabeledTriggerThresholdParams =
    std::variant<std::vector<numeric::LabeledThresholdParam>,
                 std::vector<discrete::LabeledThresholdParam>>;

using LabeledThresholdParam =
    std::variant<std::monostate, numeric::LabeledThresholdParam,
                 discrete::LabeledThresholdParam>;

inline bool
    isTriggerThresholdDiscrete(const LabeledTriggerThresholdParams& params)
{
    return std::holds_alternative<std::vector<discrete::LabeledThresholdParam>>(
        params);
}

using TriggerId = std::unique_ptr<const std::string>;
using TriggerValue = std::variant<double, std::string>;
using ThresholdName = std::optional<std::reference_wrapper<const std::string>>;

inline std::string triggerValueToString(TriggerValue val)
{
    if (auto* doubleVal = std::get_if<double>(&val); doubleVal != nullptr)
    {
        return std::to_string(*doubleVal);
    }
    else
    {
        return std::get<std::string>(val);
    }
}
