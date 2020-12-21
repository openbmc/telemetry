#pragma once

#include <sdbusplus/message/types.hpp>

#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace discrete
{

enum class Severity
{
    ok = 0,
    warning,
    critical
};

using ThresholdParam = std::tuple<std::string, std::underlying_type_t<Severity>,
                                  std::variant<double>, uint64_t>;
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

using ThresholdParam = std::tuple<std::underlying_type_t<Type>, uint64_t,
                                  std::underlying_type_t<Direction>, double>;
} // namespace numeric

using TriggerThresholdParams =
    std::variant<std::vector<numeric::ThresholdParam>,
                 std::vector<discrete::ThresholdParam>>;
