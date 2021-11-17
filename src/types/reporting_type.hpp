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
    [[noreturn]] static void throwConversionError()
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::invalid_argument),
            "Invalid reportingType");
    }
};

constexpr std::array<std::pair<std::string_view, ReportingType>, 3>
    convDataReportingType = {{std::make_pair<std::string_view, ReportingType>(
                                  "Periodic", ReportingType::periodic),
                              std::make_pair<std::string_view, ReportingType>(
                                  "OnRequest", ReportingType::onRequest),
                              std::make_pair<std::string_view, ReportingType>(
                                  "OnChange", ReportingType::onChange)}};

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
