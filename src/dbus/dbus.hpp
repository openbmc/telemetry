#pragma once

#include "core/metric.hpp"
#include "core/report.hpp"
#include "core/sensor.hpp"
#include "log.hpp"
#include "utils/utils.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/container/flat_map.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <algorithm>
#include <ctime>
#include <string>
#include <system_error>

namespace dbus
{
// D-Bus type used in APIs
namespace api
{
using MetricId = std::string;
using SensorPaths = std::vector<sdbusplus::message::object_path>;
using OperationType = std::string;
using MetricMetadata = std::string;

using ReadingValue = double;          // TODO: DOCS: Variant of what?
using ReadingTimestamp = std::string; // TODO: DOCS: Maybe it should be in raw
// formt to actually be usable, while
// formatting should be done in bmcweb?
// TODO: DOCS: Timestamp format not
// specified in documentation

} // namespace api

using MetricParams = std::tuple<api::SensorPaths, api::OperationType,
                                api::MetricId, api::MetricMetadata>;

using ReadingParam = std::tuple<api::SensorPaths, api::OperationType,
                                api::MetricId, api::MetricMetadata>;

using Reading = std::tuple<api::MetricId, api::MetricMetadata,
                           api::ReadingValue, api::ReadingTimestamp>;

namespace open_bmc
{
static constexpr auto DeleteIface = "xyz.openbmc_project.Object.Delete";
static constexpr auto DeleteMethod = "Delete";
} // namespace open_bmc

static constexpr auto Service = "xyz.openbmc_project.MonitoringService";
static constexpr auto Iface = "xyz.openbmc_project.MonitoringService";
static constexpr auto Path = "/xyz/openbmc_project/MonitoringService";

inline std::string subObject(const std::string& name)
{
    return std::string(Service) + "." + name;
}

inline std::string subIface(const std::string& name)
{
    return std::string(Iface) + "." + name;
}

inline std::string subPath(const std::string& name)
{
    return std::string(Path) + "/" + name;
}

template <class T>
static inline T toEnum(const std::vector<std::pair<T, std::string>>& convData,
                       const std::string& name)
{
    auto it =
        std::find_if(std::begin(convData), std::end(convData),
                     [&name](const auto& elem) { return elem.second == name; });

    if (it == std::end(convData))
    {
        utils::throwError(std::errc::invalid_argument, "Wrong enumerator name");
    }

    return it->first;
}

template <class T>
static inline std::vector<T>
    toEnum(const std::vector<std::pair<T, std::string>>& convData,
           const std::vector<std::string>& names)
{
    std::vector<T> result;
    result.reserve(names.size());
    std::transform(std::begin(names), std::end(names),
                   std::back_inserter(result),
                   [&convData](const std::string& name) {
                       return toEnum(convData, name);
                   });
    return result;
}

template <class T>
static inline std::string
    toString(const std::vector<std::pair<T, std::string>>& convData, T value)
{
    auto it = std::find_if(
        std::begin(convData), std::end(convData),
        [&value](const auto& elem) { return elem.first == value; });

    if (it == std::end(convData))
    {
        utils::throwError(std::errc::invalid_argument,
                          "Wrong enumerator value");
    }

    return it->second;
}

template <class T>
static inline std::vector<std::string>
    toString(const std::vector<std::pair<T, std::string>>& convData,
             const std::vector<T>& values)
{
    std::vector<std::string> result;
    result.reserve(values.size());
    std::transform(std::begin(values), std::end(values),
                   std::back_inserter(result),
                   [&convData](T value) { return toString(convData, value); });
    return result;
}

static inline core::OperationType
    getOperationType(const std::string operationType)
{
    return toEnum(core::operationTypeConvertData, operationType);
}

static inline std::string
    getOperationType(const core::OperationType operationType)
{
    return toString(core::operationTypeConvertData, operationType);
}

static inline core::ReportingType
    getReportingType(const std::string reportingType)
{
    return toEnum(core::reportingTypeConvertData, reportingType);
}

static inline std::string
    getReportingType(const core::ReportingType reportingType)
{
    return toString(core::reportingTypeConvertData, reportingType);
}

static inline std::vector<core::ReportAction>
    getReportAction(const std::vector<std::string>& reportAction)
{
    return toEnum(core::reportActionConvertData, reportAction);
}

static inline std::vector<std::string>
    getReportAction(const std::vector<core::ReportAction>& reportAction)
{
    return toString(core::reportActionConvertData, reportAction);
}

inline std::string addMissingDoubleColonToIsoTime(const char* buffer)
{
    std::string str = buffer;
    str.insert(str.size() - 2, ":");
    return str;
}

inline std::string toIsoTime(const std::time_t& timestamp)
{
    /* Format timestamp to ISO 8601 extended format with time offset */
    constexpr size_t bufferSize = sizeof("YYYY-MM-DDThh:mm:ss+hh:mm");
    std::array<char, bufferSize> buffer;

    if (std::strftime(buffer.data(), buffer.size(), "%FT%T%z",
                      std::localtime(&timestamp)))
    {
        return addMissingDoubleColonToIsoTime(buffer.data());
    }

    return "";
}

} // namespace dbus
