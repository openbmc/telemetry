#pragma once

#include <boost/serialization/strong_typedef.hpp>
#include <nlohmann/json.hpp>

#include <chrono>

BOOST_STRONG_TYPEDEF(std::chrono::milliseconds, CollectionDuration)

inline void to_json(nlohmann::json& json, const CollectionDuration& value)
{
    json = value.t.count();
}

inline void from_json(const nlohmann::json& json, CollectionDuration& value)
{
    value = CollectionDuration(std::chrono::milliseconds(json.get<uint64_t>()));
}
