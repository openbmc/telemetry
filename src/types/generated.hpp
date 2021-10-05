// This is generated file

#pragma once

#include "utils/conversion.hpp"

#include <nlohmann/json.hpp>

#include <string>
#include <vector>

namespace generated
{

enum class ReportingType
{
    OnChange,
    OnRequest,
    Periodic,
};

enum class ReportingUpdates
{
    Overwrite,
    AppendStopsWhenFull,
    AppendWrapsWhenFull,
    NewReport,
    Default,
};

enum class OperationType
{
    single,
    max,
    min,
    avg,
    sum,
};

enum class CollectionTimescope
{
    point,
    interval,
    startup,
};

struct MetricParams
{
    std::vector<std::string> sensorPaths;
    OperationType operationType;
    std::string id;
    std::string metadata;
    CollectionTimescope collectionTimescope;
    uint64_t collectionDuration;
};

struct AddReportParams
{
    std::string reportName;
    ReportingType reportingType;
    ReportingUpdates reportUpdates;
    uint64_t appendLimit;
    bool emitsReadingsUpdate;
    bool logToMetricReportsCollection;
    uint64_t interval;
    std::vector<MetricParams> metricParameters;
};

namespace details
{
constexpr std::array<std::pair<std::string_view, ReportingType>, 3>
    convDataReportingType = {
        std::make_pair("OnChange", ReportingType::OnChange),
        std::make_pair("OnRequest", ReportingType::OnRequest),
        std::make_pair("Periodic", ReportingType::Periodic)

};
}

inline ReportingType stringToReportingType(const std::string& str)
{
    return utils::stringToEnum(details::convDataReportingType, str);
}

inline std::string reportingTypeToString(ReportingType v)
{
    return std::string(utils::enumToString(details::convDataReportingType, v));
}

namespace details
{
constexpr std::array<std::pair<std::string_view, ReportingUpdates>, 5>
    convDataReportingUpdates = {
        std::make_pair("Overwrite", ReportingUpdates::Overwrite),
        std::make_pair("AppendStopsWhenFull",
                       ReportingUpdates::AppendStopsWhenFull),
        std::make_pair("AppendWrapsWhenFull",
                       ReportingUpdates::AppendWrapsWhenFull),
        std::make_pair("NewReport", ReportingUpdates::NewReport),
        std::make_pair("Default", ReportingUpdates::Default)

};
}

inline ReportingUpdates stringToReportingUpdates(const std::string& str)
{
    return utils::stringToEnum(details::convDataReportingUpdates, str);
}

inline std::string reportingUpdatesToString(ReportingUpdates v)
{
    return std::string(
        utils::enumToString(details::convDataReportingUpdates, v));
}

namespace details
{
constexpr std::array<std::pair<std::string_view, OperationType>, 5>
    convDataOperationType = {std::make_pair("single", OperationType::single),
                             std::make_pair("max", OperationType::max),
                             std::make_pair("min", OperationType::min),
                             std::make_pair("avg", OperationType::avg),
                             std::make_pair("sum", OperationType::sum)

};
}

inline OperationType stringToOperationType(const std::string& str)
{
    return utils::stringToEnum(details::convDataOperationType, str);
}

inline std::string operationTypeToString(OperationType v)
{
    return std::string(utils::enumToString(details::convDataOperationType, v));
}

namespace details
{
constexpr std::array<std::pair<std::string_view, CollectionTimescope>, 3>
    convDataCollectionTimescope = {
        std::make_pair("point", CollectionTimescope::point),
        std::make_pair("interval", CollectionTimescope::interval),
        std::make_pair("startup", CollectionTimescope::startup)

};
}

inline CollectionTimescope stringToCollectionTimescope(const std::string& str)
{
    return utils::stringToEnum(details::convDataCollectionTimescope, str);
}

inline std::string collectionTimescopeToString(CollectionTimescope v)
{
    return std::string(
        utils::enumToString(details::convDataCollectionTimescope, v));
}

} // namespace generated

namespace nlohmann
{

template <>
struct adl_serializer<generated::ReportingType>
{
    static void from_json(const json& j, generated::ReportingType& value)
    {
        value = generated::stringToReportingType(j.get<std::string>());
    }
};

template <>
struct adl_serializer<generated::ReportingUpdates>
{
    static void from_json(const json& j, generated::ReportingUpdates& value)
    {
        value = generated::stringToReportingUpdates(j.get<std::string>());
    }
};

template <>
struct adl_serializer<generated::OperationType>
{
    static void from_json(const json& j, generated::OperationType& value)
    {
        value = generated::stringToOperationType(j.get<std::string>());
    }
};

template <>
struct adl_serializer<generated::CollectionTimescope>
{
    static void from_json(const json& j, generated::CollectionTimescope& value)
    {
        value = generated::stringToCollectionTimescope(j.get<std::string>());
    }
};

template <>
struct adl_serializer<generated::MetricParams>
{
    static void from_json(const json& j, generated::MetricParams& value)
    {
        j.at("sensorPaths").get_to(value.sensorPaths);
        if (auto prop = j.find("operationType"); prop != j.end())
        {
            prop->get_to(value.operationType);
        }
        else
        {
            json("single").get_to(value.operationType);
        }
        j.at("id").get_to(value.id);
        j.at("metadata").get_to(value.metadata);
        if (auto prop = j.find("collectionTimescope"); prop != j.end())
        {
            prop->get_to(value.collectionTimescope);
        }
        else
        {
            json("point").get_to(value.collectionTimescope);
        }
        if (auto prop = j.find("collectionDuration"); prop != j.end())
        {
            prop->get_to(value.collectionDuration);
        }
        else
        {
            json(123).get_to(value.collectionDuration);
        }
    }
};

template <>
struct adl_serializer<generated::AddReportParams>
{
    static void from_json(const json& j, generated::AddReportParams& value)
    {
        j.at("reportName").get_to(value.reportName);
        j.at("reportingType").get_to(value.reportingType);
        if (auto prop = j.find("reportUpdates"); prop != j.end())
        {
            prop->get_to(value.reportUpdates);
        }
        else
        {
            json("Overwrite").get_to(value.reportUpdates);
        }
        if (auto prop = j.find("appendLimit"); prop != j.end())
        {
            prop->get_to(value.appendLimit);
        }
        else
        {
            json(255).get_to(value.appendLimit);
        }
        j.at("emitsReadingsUpdate").get_to(value.emitsReadingsUpdate);
        j.at("logToMetricReportsCollection")
            .get_to(value.logToMetricReportsCollection);
        j.at("interval").get_to(value.interval);
        j.at("metricParameters").get_to(value.metricParameters);
    }
};

} // namespace nlohmann
