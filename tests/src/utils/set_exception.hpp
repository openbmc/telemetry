#pragma once

#include <future>

namespace utils
{

template <class T>
inline void setException(std::promise<T>& promise, const std::string& message)
{
    try
    {
        throw std::runtime_error(message);
    }
    catch (const std::runtime_error&)
    {
        promise.set_exception(std::current_exception());
    }
}

} // namespace utils
