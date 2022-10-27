#pragma once

#include "utils/conversion.hpp"

#include <sdbusplus/exception.hpp>

#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>

enum class ReportingType : uint32_t
{
    periodic,
    onRequest,
    onChange
};

namespace utils
{

template <>
struct EnumTraits<ReportingType>
{
    static constexpr auto propertyName = ConstexprString{"ReportingType"};
};

constexpr std::array<std::pair<std::string_view, ReportingType>, 3>
    convDataReportingType = {
        {std::make_pair<std::string_view, ReportingType>(
             "xyz.openbmc_project.Telemetry.Report.ReportingType.OnChange",
             ReportingType::onChange),
         std::make_pair<std::string_view, ReportingType>(
             "xyz.openbmc_project.Telemetry.Report.ReportingType.OnRequest",
             ReportingType::onRequest),
         std::make_pair<std::string_view, ReportingType>(
             "xyz.openbmc_project.Telemetry.Report.ReportingType.Periodic",
             ReportingType::periodic)}};

inline ReportingType
    toReportingType(std::underlying_type_t<ReportingType> value)
{
    return toEnum<ReportingType, minEnumValue(convDataReportingType),
                  maxEnumValue(convDataReportingType)>(value);
}

inline ReportingType toReportingType(const std::string& value)
{
    return toEnum(convDataReportingType, value);
}

inline std::string enumToString(ReportingType value)
{
    return std::string(enumToString(convDataReportingType, value));
}

} // namespace utils
