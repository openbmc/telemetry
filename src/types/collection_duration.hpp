#pragma once

#include "types/duration_type.hpp"

#include <boost/serialization/strong_typedef.hpp>
#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdint>

BOOST_STRONG_TYPEDEF(DurationType, CollectionDuration)

inline void to_json(nlohmann::json& json, const CollectionDuration& value)
{
    json = value.t.count();
}

inline void from_json(const nlohmann::json& json, CollectionDuration& value)
{
    value = CollectionDuration(DurationType(json.get<uint64_t>()));
}
