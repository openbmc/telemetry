#pragma once

#include <nlohmann/json.hpp>

#include <string_view>

namespace utils
{

template <class T>
struct is_vector : std::false_type
{};

template <class T>
struct is_vector<std::vector<T>> : std::true_type
{};

template <class T>
constexpr bool is_vector_v = is_vector<T>::value;

template <class T>
std::optional<T> readJson(const nlohmann::json& json)
{
    if constexpr (is_vector_v<T>)
    {
        if (json.is_array())
        {
            auto result = T{};
            for (const auto& item : json.items())
            {
                if (auto val = readJson<typename T::value_type>(item.value()))
                {
                    result.emplace_back(*val);
                }
            }
            return result;
        }
    }
    else
    {
        if (const T* val = json.get_ptr<const T*>())
        {
            return *val;
        }
    }

    return std::nullopt;
}

template <class T>
std::optional<T> readJson(const nlohmann::json& json, std::string_view key)
{
    auto it = json.find(key);
    if (it != json.end())
    {
        const nlohmann::json& subJson = *it;
        return readJson<T>(subJson);
    }

    return std::nullopt;
}

} // namespace utils
