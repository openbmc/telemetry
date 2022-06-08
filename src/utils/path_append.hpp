#pragma once

#include <sdbusplus/message.hpp>

#include <ranges>
#include <string_view>

namespace utils
{

inline sdbusplus::message::object_path
    pathAppend(sdbusplus::message::object_path path,
               const std::string& extension)
{
    size_t pos_start = 0;
    size_t pos_end = 0;
    while ((pos_end = extension.find('/', pos_start)) != std::string::npos)
    {
        path /= std::string_view(extension.begin() + pos_start,
                                 extension.begin() + pos_end);
        pos_start = pos_end + 1;
    }
    path /= std::string_view(extension.begin() + pos_start, extension.end());
    return path;
}

} // namespace utils
