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
    appendWrapsWhenFull,
    newReport
};

namespace utils
{

template <>
struct EnumTraits<ReportUpdates>
{
    [[noreturn]] static void throwConversionError()
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::invalid_argument),
            "Invalid ReportUpdates");
    }
};

constexpr std::array<std::pair<std::string_view, ReportUpdates>, 4>
    convDataReportUpdates = {
        {std::make_pair<std::string_view, ReportUpdates>(
             "Overwrite", ReportUpdates::overwrite),
         std::make_pair<std::string_view, ReportUpdates>(
             "AppendStopsWhenFull", ReportUpdates::appendStopsWhenFull),
         std::make_pair<std::string_view, ReportUpdates>(
             "AppendWrapsWhenFull", ReportUpdates::appendWrapsWhenFull),
         std::make_pair<std::string_view, ReportUpdates>(
             "NewReport", ReportUpdates::newReport)}};

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
