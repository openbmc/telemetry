#pragma once

#include <future>

namespace utils
{

template <class T>
inline void setException(std::promise<T>& promise, const std::string& message)
{
    promise.set_exception(std::make_exception_ptr(std::runtime_error(message)));
}

} // namespace utils
