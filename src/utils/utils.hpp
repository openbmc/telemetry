#pragma once

#include "log.hpp"

#include <sdbusplus/exception.hpp>

#include <memory>
#include <string>
#include <system_error>

namespace utils
{

template <class T, template <class> class U>
std::weak_ptr<T> weak_ptr(const U<T>& ptr)
{
    return ptr;
}

[[noreturn]] static inline void throwError(const std::errc e,
                                           const std::string& msg)
{
    LOG_ERROR << "api error: " << std::make_error_code(e) << " : " << msg;
    throw sdbusplus::exception::SdBusError(static_cast<int>(e), msg.c_str());
}

} // namespace utils
