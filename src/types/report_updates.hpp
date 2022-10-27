#pragma once

#include "utils/conversion.hpp"

#include <sdbusplus/exception.hpp>

#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>

enum class ReportUpdates : uint32_t
{
    overwrite,
    appendStopsWhenFull,
    appendWrapsWhenFull
};

namespace utils
{

template <>
struct EnumTraits<ReportUpdates>
{
    static constexpr auto propertyName = ConstexprString{"ReportUpdates"};
};

constexpr auto convDataReportUpdates = std::array{
    std::make_pair<std::string_view, ReportUpdates>(
        "xyz.openbmc_project.Telemetry.Report.ReportUpdates.Overwrite",
        ReportUpdates::overwrite),
    std::make_pair<std::string_view, ReportUpdates>(
        "xyz.openbmc_project.Telemetry.Report.ReportUpdates."
        "AppendStopsWhenFull",
        ReportUpdates::appendStopsWhenFull),
    std::make_pair<std::string_view, ReportUpdates>(
        "xyz.openbmc_project.Telemetry.Report.ReportUpdates."
        "AppendWrapsWhenFull",
        ReportUpdates::appendWrapsWhenFull)};

inline ReportUpdates
    toReportUpdates(std::underlying_type_t<ReportUpdates> value)
{
    return toEnum<ReportUpdates, minEnumValue(convDataReportUpdates),
                  maxEnumValue(convDataReportUpdates)>(value);
}

inline ReportUpdates toReportUpdates(const std::string& value)
{
    return toEnum(convDataReportUpdates, value);
}

inline std::string enumToString(ReportUpdates value)
{
    return std::string(enumToString(convDataReportUpdates, value));
}

} // namespace utils
