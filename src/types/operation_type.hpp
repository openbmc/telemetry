#pragma once

#include "utils/conversion.hpp"

#include <array>
#include <cstdint>
#include <string_view>

enum class OperationType : uint32_t
{
    single,
    max,
    min,
    avg,
    sum
};

namespace utils
{

constexpr std::array<std::pair<std::string_view, OperationType>, 5>
    convDataOperationType = {
        {std::make_pair<std::string_view, OperationType>("SINGLE",
                                                         OperationType::single),
         std::make_pair<std::string_view, OperationType>("Maximum",
                                                         OperationType::max),
         std::make_pair<std::string_view, OperationType>("Minimum",
                                                         OperationType::min),
         std::make_pair<std::string_view, OperationType>("Average",
                                                         OperationType::avg),
         std::make_pair<std::string_view, OperationType>("Summation",
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
