#pragma once

#include "log.hpp"

#include <sdbusplus/exception.hpp>

#include <memory>
#include <string>
#include <system_error>

namespace utils
{

[[noreturn]] inline void throwError(const std::errc e, const std::string& msg)
{
    throw sdbusplus::exception::SdBusError(static_cast<int>(e), msg.c_str());
}

} // namespace utils

#define apiError(e, msg)                                                       \
    LOG_ERROR << std::make_error_code(e) << " : " << msg;                      \
    utils::throwError(e, msg);
