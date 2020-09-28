#pragma once

#include "log.hpp"

#include <sdbusplus/exception.hpp>

#include <system_error>

#define sdBusError(e, msg)                                                     \
    (LOG_ERROR << "SdBusError with errc: " << std::make_error_code(e) << " - " \
               << msg,                                                         \
     sdbusplus::exception::SdBusError(static_cast<int>(e), msg))
