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

using DiscreteThreshold =
    std::tuple<std::string, std::string, std::variant<double>, uint64_t>;
using DiscreteThresholds =
    std::tuple<std::string, std::vector<DiscreteThreshold>>;

using NumericThreshold = std::tuple<std::string, uint64_t, std::string, double>;
using TriggerThresholds =
    std::variant<DiscreteThresholds, std::vector<NumericThreshold>>;
