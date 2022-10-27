#pragma once

#include "utils/conversion.hpp"

#include <array>
#include <cstdint>
#include <string_view>

enum class OperationType : uint32_t
{
    max,
    min,
    avg,
    sum
};

namespace utils
{

template <>
struct EnumTraits<OperationType>
{
    static constexpr auto propertyName = ConstexprString{"OperationType"};
};

constexpr std::array<std::pair<std::string_view, OperationType>, 4>
    convDataOperationType = {
        {std::make_pair<std::string_view, OperationType>(
             "xyz.openbmc_project.Telemetry.Report.OperationType.Maximum",
             OperationType::max),
         std::make_pair<std::string_view, OperationType>(
             "xyz.openbmc_project.Telemetry.Report.OperationType.Minimum",
             OperationType::min),
         std::make_pair<std::string_view, OperationType>(
             "xyz.openbmc_project.Telemetry.Report.OperationType.Average",
             OperationType::avg),
         std::make_pair<std::string_view, OperationType>(
             "xyz.openbmc_project.Telemetry.Report.OperationType.Summation",
             OperationType::sum)}};

inline OperationType
    toOperationType(std::underlying_type_t<OperationType> value)
{
    return toEnum<OperationType, minEnumValue(convDataOperationType),
                  maxEnumValue(convDataOperationType)>(value);
}

inline OperationType toOperationType(const std::string& value)
{
    return toEnum(convDataOperationType, value);
}

inline std::string enumToString(OperationType value)
{
    return std::string(enumToString(convDataOperationType, value));
}

} // namespace utils
