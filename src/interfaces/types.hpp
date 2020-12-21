#pragma once

#include "utils/labeled_tuple.hpp"
#include "utils/tstring.hpp"

#include <sdbusplus/message/types.hpp>

#include <string>
#include <tuple>
#include <vector>

using ReadingParameters =
    std::vector<std::tuple<std::vector<sdbusplus::message::object_path>,
                           std::string, std::string, std::string>>;

using LabeledSensorParameters =
    utils::LabeledTuple<std::tuple<std::string, std::string>,
                        utils::tstring::Service, utils::tstring::Path>;

using LabeledMetricParameters =
    utils::LabeledTuple<std::tuple<std::vector<LabeledSensorParameters>,
                                   std::string, std::string, std::string>,
                        utils::tstring::SensorPaths,
                        utils::tstring::OperationType, utils::tstring::Id,
                        utils::tstring::MetricMetadata>;

using Readings = std::tuple<
    uint64_t,
    std::vector<std::tuple<std::string, std::string, double, uint64_t>>>;

namespace discrete
{

enum Condition : int
{
    changed = 0,
    specified
};

enum Severity : int
{
    ok = 0,
    warning,
    critical
};

using ThresholdParam = std::tuple<std::string, std::underlying_type_t<Severity>,
                                  std::variant<double>, uint64_t>;
using ThresholdParams =
    std::tuple<std::underlying_type_t<Condition>, std::vector<ThresholdParam>>;
} // namespace discrete

namespace numeric
{

enum Level : int
{
    lowerCritical = 0,
    lowerWarning,
    upperWarning,
    upperCritical
};

enum Direction : int
{
    either = 0,
    decreasing,
    increasing
};

using ThresholdParam = std::tuple<std::underlying_type_t<Level>, uint64_t,
                                  std::underlying_type_t<Direction>, double>;
} // namespace numeric

using TriggerThresholdParams =
    std::variant<discrete::ThresholdParams,
                 std::vector<numeric::ThresholdParam>>;
