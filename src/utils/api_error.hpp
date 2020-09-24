#pragma once

#include "log.hpp"

#include <sdbusplus/exception.hpp>

#include <string>
#include <system_error>

#define sdBusError(e, msg)                                                     \
    (LOG_ERROR << "SdBusError with errc: " << std::make_error_code(e) << " - " \
               << msg,                                                         \
     sdbusplus::exception::SdBusError(static_cast<int>(e), msg))

#define sdBusErrorStr(e, msg)                                                  \
    (LOG_ERROR << "SdBusError with errc: " << std::make_error_code(e) << " - " \
               << msg,                                                         \
     sdbusplus::exception::SdBusError(static_cast<int>(e), (msg).c_str()))
