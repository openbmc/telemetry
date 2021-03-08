#pragma once

#include "types/collection_duration.hpp"
#include "types/collection_time_scope.hpp"
#include "types/operation_type.hpp"
#include "utils/labeled_tuple.hpp"
#include "utils/tstring.hpp"

#include <sdbusplus/message/types.hpp>

#include <chrono>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

using ReadingParametersPastVersion =
    std::vector<std::tuple<sdbusplus::message::object_path, std::string,
                           std::string, std::string>>;

using ReadingParameters = std::vector<
    std::tuple<std::vector<sdbusplus::message::object_path>, std::string,
               std::string, std::string, std::string, uint64_t>>;

using LabeledSensorParameters =
    utils::LabeledTuple<std::tuple<std::string, std::string>,
                        utils::tstring::Service, utils::tstring::Path>;

using LabeledMetricParameters = utils::LabeledTuple<
    std::tuple<std::vector<LabeledSensorParameters>, OperationType, std::string,
               std::string, CollectionTimeScope, CollectionDuration>,
    utils::tstring::SensorPath, utils::tstring::OperationType,
    utils::tstring::Id, utils::tstring::MetricMetadata,
    utils::tstring::CollectionTimeScope, utils::tstring::CollectionDuration>;

using Readings = std::tuple<
    uint64_t,
    std::vector<std::tuple<std::string, std::string, double, uint64_t>>>;

ReadingParameters
    toReadingParameters(const std::vector<LabeledMetricParameters>& labeled);
