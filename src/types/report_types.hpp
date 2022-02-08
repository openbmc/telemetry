#pragma once

#include "types/collection_duration.hpp"
#include "types/collection_time_scope.hpp"
#include "types/operation_type.hpp"
#include "types/sensor_types.hpp"
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

using ReadingParameters = std::vector<std::tuple<
    std::vector<std::tuple<sdbusplus::message::object_path, std::string>>,
    std::string, std::string, std::string, uint64_t>>;

using LabeledMetricParameters = utils::LabeledTuple<
    std::tuple<std::vector<LabeledSensorInfo>, OperationType, std::string,
               CollectionTimeScope, CollectionDuration>,
    utils::tstring::SensorPath, utils::tstring::OperationType,
    utils::tstring::Id, utils::tstring::CollectionTimeScope,
    utils::tstring::CollectionDuration>;

using ReadingData = std::tuple<std::string, std::string, double, uint64_t>;

using Readings = std::tuple<uint64_t, std::vector<ReadingData>>;

ReadingParameters
    toReadingParameters(const std::vector<LabeledMetricParameters>& labeled);
