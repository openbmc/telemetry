#pragma once

#include "utils/conversion.hpp"

#include <sdbusplus/exception.hpp>

#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>

enum class ErrorType : uint32_t
{
    propertyConflict
};

namespace utils
{

constexpr auto convDataErrorType =
    std::array{std::make_pair<std::string_view, ErrorType>(
        "PropertyConflict", ErrorType::propertyConflict)};

inline ErrorType toErrorType(std::underlying_type_t<ErrorType> value)
{
    return toEnum<ErrorType, minEnumValue(convDataErrorType),
                  maxEnumValue(convDataErrorType)>(value);
}

inline ErrorType toErrorType(const std::string& value)
{
    return toEnum(convDataErrorType, value);
}

inline std::string enumToString(ErrorType value)
{
    return std::string(enumToString(convDataErrorType, value));
}

} // namespace utils
