#pragma once

#include "utils/conversion.hpp"

#include <cstdint>
#include <string>
#include <vector>

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

const std::vector<std::pair<std::string, OperationType>> convDataOperationType =
    {{std::make_pair<std::string, OperationType>("SINGLE",
                                                 OperationType::single),
      std::make_pair<std::string, OperationType>("MAX", OperationType::max),
      std::make_pair<std::string, OperationType>("MIN", OperationType::min),
      std::make_pair<std::string, OperationType>("AVG", OperationType::avg),
      std::make_pair<std::string, OperationType>("SUM", OperationType::sum)}};

inline OperationType
    toOperationType(std::underlying_type_t<OperationType> value)
{
    return toEnum<OperationType, OperationType::single, OperationType::sum>(
        value);
}

inline OperationType stringToOperationType(const std::string& value)
{
    return stringToEnum(convDataOperationType, value);
}

inline std::string enumToString(OperationType value)
{
    return enumToString(convDataOperationType, value);
}

} // namespace utils
