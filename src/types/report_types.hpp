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

using ReadingData = std::tuple<std::string, std::string, double, uint64_t>;

using Readings = std::tuple<uint64_t, std::vector<ReadingData>>;

ReadingParameters
    toReadingParameters(const std::vector<LabeledMetricParameters>& labeled);

enum class UpdatePolicy
{
    Overwrite = 0,
    AppendStopWhenFull,
    AppendWrapWhenFull
};

namespace details
{
constexpr std::array<std::pair<std::string_view, UpdatePolicy>, 3>
    convDataUpdatePolicy = {
        std::make_pair("Overwrite", UpdatePolicy::Overwrite),
        std::make_pair("AppendStopWhenFull", UpdatePolicy::AppendStopWhenFull),
        std::make_pair("AppendWrapWhenFull", UpdatePolicy::AppendWrapWhenFull)};

} // namespace details

inline UpdatePolicy stringToUpdatePolicy(const std::string& str)
{
    return utils::stringToEnum(details::convDataUpdatePolicy, str);
}

inline std::string updatePolicyToString(UpdatePolicy v)
{
    return std::string(utils::enumToString(details::convDataUpdatePolicy, v));
}