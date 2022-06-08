#pragma once
#include <sdbusplus/message.hpp>

#include <gmock/gmock.h>

namespace sdbusplus::message::details
{
inline void PrintTo(const string_path_wrapper& path, std::ostream* os)
{
    *os << path.str;
}
} // namespace sdbusplus::message::details
