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

constexpr std::array<std::pair<std::string_view, CollectionTimeScope>, 3>
    convDataCollectionTimeScope = {
        {std::make_pair<std::string_view, CollectionTimeScope>(
             "Point", CollectionTimeScope::point),
         std::make_pair<std::string_view, CollectionTimeScope>(
             "Interval", CollectionTimeScope::interval),
         std::make_pair<std::string_view, CollectionTimeScope>(
             "StartupInterval", CollectionTimeScope::startup)}};

inline CollectionTimeScope
    toCollectionTimeScope(std::underlying_type_t<CollectionTimeScope> value)
{
    return toEnum<CollectionTimeScope, CollectionTimeScope::point,
                  CollectionTimeScope::startup>(value);
}

inline CollectionTimeScope stringToCollectionTimeScope(const std::string& value)
{
    return stringToEnum(convDataCollectionTimeScope, value);
}

inline std::string enumToString(CollectionTimeScope value)
{
    return std::string(enumToString(convDataCollectionTimeScope, value));
}

} // namespace utils