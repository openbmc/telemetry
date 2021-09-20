#pragma once

#include <boost/type_index.hpp>

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string>

namespace utils
{

template <class T, T first, T last>
inline T toEnum(std::underlying_type_t<T> x)
{
    if (x < static_cast<std::underlying_type_t<T>>(first) ||
        x > static_cast<std::underlying_type_t<T>>(last))
    {
        throw std::out_of_range("Value \"" + std::to_string(x) +
                                "\" is not in range of " +
                                boost::typeindex::type_id<T>().pretty_name());
    }
    return static_cast<T>(x);
}

template <class T>
inline std::underlying_type_t<T> toUnderlying(T value)
{
    return static_cast<std::underlying_type_t<T>>(value);
}

template <class T, size_t N>
inline T stringToEnum(const std::array<std::pair<std::string_view, T>, N>& data,
                      const std::string& value)
{
    auto it = std::find_if(
        std::begin(data), std::end(data),
        [&value](const auto& item) { return item.first == value; });
    if (it == std::end(data))
    {
        throw std::out_of_range("Value \"" + std::string(value) +
                                "\" is not in range of " +
                                boost::typeindex::type_id<T>().pretty_name());
    }
    return it->second;
}

template <class T, size_t N>
inline std::string_view
    enumToString(const std::array<std::pair<std::string_view, T>, N>& data,
                 T value)
{
    auto it = std::find_if(
        std::begin(data), std::end(data),
        [value](const auto& item) { return item.second == value; });
    if (it == std::end(data))
    {
        throw std::out_of_range("Value \"" +
                                std::to_string(toUnderlying(value)) +
                                "\" is not in range of " +
                                boost::typeindex::type_id<T>().pretty_name());
    }
    return it->first;
}

} // namespace utils
