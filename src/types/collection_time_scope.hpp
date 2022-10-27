#pragma once

#include "utils/conversion.hpp"

#include <array>
#include <cstdint>
#include <string_view>
#include <type_traits>

enum class CollectionTimeScope : uint32_t
{
    point,
    interval,
    startup
};

namespace utils
{

template <>
struct EnumTraits<CollectionTimeScope>
{
    static constexpr auto propertyName = ConstexprString{"CollectionTimeScope"};
};

constexpr std::array<std::pair<std::string_view, CollectionTimeScope>, 3>
    convDataCollectionTimeScope = {
        {std::make_pair<std::string_view, CollectionTimeScope>(
             "xyz.openbmc_project.Telemetry.Report.CollectionTimescope.Point",
             CollectionTimeScope::point),
         std::make_pair<std::string_view, CollectionTimeScope>(
             "xyz.openbmc_project.Telemetry.Report.CollectionTimescope."
             "Interval",
             CollectionTimeScope::interval),
         std::make_pair<std::string_view, CollectionTimeScope>(
             "xyz.openbmc_project.Telemetry.Report.CollectionTimescope."
             "StartupInterval",
             CollectionTimeScope::startup)}};

inline CollectionTimeScope
    toCollectionTimeScope(std::underlying_type_t<CollectionTimeScope> value)
{
    return toEnum<CollectionTimeScope,
                  minEnumValue(convDataCollectionTimeScope),
                  maxEnumValue(convDataCollectionTimeScope)>(value);
}

inline CollectionTimeScope toCollectionTimeScope(const std::string& value)
{
    return toEnum(convDataCollectionTimeScope, value);
}

inline std::string enumToString(CollectionTimeScope value)
{
    return std::string(enumToString(convDataCollectionTimeScope, value));
}

} // namespace utils
