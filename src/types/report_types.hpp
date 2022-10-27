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
#include <variant>
#include <vector>

using ReadingParameters = std::vector<std::tuple<
    std::vector<std::tuple<sdbusplus::message::object_path, std::string>>,
    std::string, std::string, uint64_t>>;

using LabeledMetricParameters = utils::LabeledTuple<
    std::tuple<std::vector<LabeledSensorInfo>, OperationType,
               CollectionTimeScope, CollectionDuration>,
    utils::tstring::SensorPath, utils::tstring::OperationType,
    utils::tstring::CollectionTimeScope, utils::tstring::CollectionDuration>;

using AddReportVariant =
    std::variant<std::monostate, bool, uint64_t, std::string,
                 std::vector<std::string>, ReadingParameters>;

ReadingParameters
    toReadingParameters(const std::vector<LabeledMetricParameters>& labeled);
