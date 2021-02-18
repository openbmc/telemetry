#pragma once

#include "utils/conversion.hpp"
#include "utils/labeled_tuple.hpp"
#include "utils/tstring.hpp"

#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

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
    convDataSeverity = {std::make_pair("Ok", Severity::ok),
                        std::make_pair("Warning", Severity::warning),
                        std::make_pair("Critical", Severity::critical)};

} // namespace details

inline Severity stringToSeverity(const std::string& str)
{
    return utils::stringToEnum(details::convDataSeverity, str);
}

inline std::string severityToString(Severity v)
{
    return std::string(utils::enumToString(details::convDataSeverity, v));
}

using ThresholdParam = std::tuple<std::string, std::string, double, uint64_t>;

using LabeledThresholdParam =
    utils::LabeledTuple<std::tuple<std::string, Severity, double, uint64_t>,
                        utils::tstring::UserId, utils::tstring::Severity,
                        utils::tstring::DwellTime,
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

inline Type stringToType(const std::string& str)
{
    return utils::stringToEnum(details::convDataType, str);
}

inline std::string typeToString(Type v)
{
    return std::string(utils::enumToString(details::convDataType, v));
}

inline Direction stringToDirection(const std::string& str)
{
    return utils::stringToEnum(details::convDataDirection, str);
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

using TriggerSensors =
    std::vector<std::pair<sdbusplus::message::object_path, std::string>>;

using LabeledTriggerSensor =
    utils::LabeledTuple<std::tuple<std::string, std::string>,
                        utils::tstring::SensorPath,
                        utils::tstring::SensorMetadata>;

using TriggerThresholdParams =
    std::variant<std::vector<numeric::ThresholdParam>,
                 std::vector<discrete::ThresholdParam>>;

using LabeledTriggerThresholdParams =
    std::variant<std::vector<numeric::LabeledThresholdParam>,
                 std::vector<discrete::LabeledThresholdParam>>;