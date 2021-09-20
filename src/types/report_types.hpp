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

enum class ReportUpdates
{
    Overwrite = 0,
    AppendStopWhenFull,
    AppendWrapWhenFull,
    NewReport,
    Default
};

namespace details
{
constexpr std::array<std::pair<std::string_view, ReportUpdates>, 5>
    convDataReportUpdates = {
        std::make_pair("Overwrite", ReportUpdates::Overwrite),
        std::make_pair("AppendStopWhenFull", ReportUpdates::AppendStopWhenFull),
        std::make_pair("AppendWrapWhenFull", ReportUpdates::AppendWrapWhenFull),
        std::make_pair("NewReport", ReportUpdates::NewReport),
        std::make_pair("Default", ReportUpdates::Default)};

} // namespace details

inline ReportUpdates stringToReportUpdates(const std::string& str)
{
    return utils::stringToEnum(details::convDataReportUpdates, str);
}

inline std::string reportUpdatesToString(ReportUpdates v)
{
    return std::string(utils::enumToString(details::convDataReportUpdates, v));
}

enum class ReportingType
{
    OnChange = 0,
    OnRequest,
    Periodic
};

namespace details
{
constexpr std::array<std::pair<std::string_view, ReportingType>, 3>
    convDataReportingType = {
        std::make_pair("OnChange", ReportingType::OnChange),
        std::make_pair("OnRequest", ReportingType::OnRequest),
        std::make_pair("Periodic", ReportingType::Periodic)};

} // namespace details

inline ReportingType stringToReportingType(const std::string& str)
{
    return utils::stringToEnum(details::convDataReportingType, str);
}

inline std::string reportingTypeToString(ReportingType v)
{
    return std::string(utils::enumToString(details::convDataReportingType, v));
}
