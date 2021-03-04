#pragma once

#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdint>

using CollectionDuration = std::chrono::duration<uint64_t, std::milli>;

namespace utils
{

inline void to_json(nlohmann::json& json, const CollectionDuration& value)
{
    json = value.count();
}

inline void from_json(const nlohmann::json& json, CollectionDuration& value)
{
    value = CollectionDuration(json.get<uint64_t>());
}

} // namespace utils
