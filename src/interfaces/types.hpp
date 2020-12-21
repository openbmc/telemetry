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

enum DiscreteConditionType : int
{
    changed = 0,
    specified
};

enum DiscreteSeverityType : int
{
    ok = 0,
    warning,
    critical
};

using DiscreteThresholdParam =
    std::tuple<std::string, std::underlying_type_t<DiscreteSeverityType>,
               std::variant<double>, uint64_t>;
using DiscreteThresholdParams =
    std::tuple<std::underlying_type_t<DiscreteConditionType>,
               std::vector<DiscreteThresholdParam>>;

enum NumericThresholdType : int
{
    lowerCritical = 0,
    lowerWarning,
    upperWarning,
    upperCritical
};

enum NumericActivationType : int
{
    either = 0,
    decreasing,
    increasing
};

using NumericThresholdParam =
    std::tuple<std::underlying_type_t<NumericThresholdType>, uint64_t,
               std::underlying_type_t<NumericActivationType>, double>;
using TriggerThresholdParams =
    std::variant<DiscreteThresholdParams, std::vector<NumericThresholdParam>>;
