#pragma once

#include "utils/conversion.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>

enum class ReportAction : uint32_t
{
    emitsReadingsUpdate,
    logToMetricReportsCollection
};

namespace utils
{

template <>
struct EnumTraits<ReportAction>
{
    static constexpr auto propertyName = ConstexprString{"ReportAction"};
};

constexpr std::array<std::pair<std::string_view, ReportAction>, 2>
    convDataReportAction = {{std::make_pair<std::string_view, ReportAction>(
                                 "xyz.openbmc_project.Telemetry.Report."
                                 "ReportActions.EmitsReadingsUpdate",
                                 ReportAction::emitsReadingsUpdate),
                             std::make_pair<std::string_view, ReportAction>(
                                 "xyz.openbmc_project.Telemetry.Report."
                                 "ReportActions.LogToMetricReportsCollection",
                                 ReportAction::logToMetricReportsCollection)}};

inline ReportAction toReportAction(std::underlying_type_t<ReportAction> value)
{
    return toEnum<ReportAction, minEnumValue(convDataReportAction),
                  maxEnumValue(convDataReportAction)>(value);
}

inline ReportAction toReportAction(const std::string& value)
{
    return toEnum(convDataReportAction, value);
}

inline std::string enumToString(ReportAction value)
{
    return std::string(enumToString(convDataReportAction, value));
}

} // namespace utils
