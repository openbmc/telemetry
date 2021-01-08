#pragma once

#include <stdexcept>

namespace utils
{

template <class T, T first, T last>
inline T toEnum(int x)
{
    if (x < static_cast<decltype(x)>(first) ||
        x > static_cast<decltype(x)>(last))
    {
        throw std::out_of_range("Value is not in range of enum");
    }
    return static_cast<T>(x);
}
} // namespace utils
