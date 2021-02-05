#pragma once

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

namespace utils
{

template <class T, T first, T last>
inline T toEnum(std::underlying_type_t<T> x)
{
    if (x < static_cast<std::underlying_type_t<T>>(first) ||
        x > static_cast<std::underlying_type_t<T>>(last))
    {
        throw std::out_of_range("Value is not in range of enum");
    }
    return static_cast<T>(x);
}

template <class T>
inline std::underlying_type_t<T> toUnderlying(T value)
{
    return static_cast<std::underlying_type_t<T>>(value);
}

template <class T>
inline T stringToEnum(const std::vector<std::pair<std::string, T>>& data,
                      const std::string& value)
{
    auto it = std::find_if(
        std::begin(data), std::end(data),
        [&value](const auto& item) { return item.first == value; });
    if (it == std::end(data))
    {
        throw std::out_of_range("Value is not in range of enum");
    }
    return it->second;
}

template <class T>
inline std::string
    enumToString(const std::vector<std::pair<std::string, T>>& data, T value)
{
    auto it = std::find_if(
        std::begin(data), std::end(data),
        [value](const auto& item) { return item.second == value; });
    if (it == std::end(data))
    {
        throw std::out_of_range("Value is not in range of enum");
    }
    return it->first;
}

} // namespace utils
