#pragma once

#include <chrono>

namespace utils
{

template <class T, class Functor>
inline T measureElapsedTime(Functor&& functor)
{
    auto start = std::chrono::high_resolution_clock::now();

    functor();

    auto end = std::chrono::high_resolution_clock::now();

    return std::chrono::duration_cast<T>(end - start);
}

} // namespace utils
