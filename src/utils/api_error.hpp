#pragma once

#include "log.hpp"

#include <sdbusplus/exception.hpp>

#include <string>
#include <system_error>

namespace details
{

inline sdbusplus::exception::SdBusError sdBusError(const char* file,
                                                   const size_t line,
                                                   std::errc e,
                                                   const std::string& msg)
{
    utils::Logger<utils::LogLevel::Error>("SdBusError", file, line)
        << "[errc= " << static_cast<int>(e) << "] " << msg;
    return sdbusplus::exception::SdBusError(static_cast<int>(e), msg.c_str());
}

} // namespace details

#define sdBusError(e, msg) details::sdBusError(__FILE__, __LINE__, e, msg)
