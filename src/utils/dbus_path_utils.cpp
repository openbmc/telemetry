#include "utils/dbus_path_utils.hpp"

namespace utils
{
sdbusplus::message::object_path pathAppend(sdbusplus::message::object_path path,
                                           const std::string& appended)
{
    if (appended.starts_with('/') || !isValidDbusPath(appended))
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::invalid_argument),
            "Invalid appended string");
    }

    size_t pos_start = 0;
    size_t pos_end = 0;
    while ((pos_end = appended.find('/', pos_start)) != std::string::npos)
    {
        if (pos_start == pos_end)
        {
            throw sdbusplus::exception::SdBusError(
                static_cast<int>(std::errc::invalid_argument),
                "Invalid appended string");
        }
        path /= std::string_view(appended.begin() + pos_start,
                                 appended.begin() + pos_end);
        pos_start = pos_end + 1;
    }
    path /= std::string_view(appended.begin() + pos_start, appended.end());
    return path;
}

std::string reportPathToId(const sdbusplus::message::object_path& path)
{
    if (path.str.starts_with(constants::reportDirStr))
    {
        auto id = path.str.substr(constants::reportDirStr.length());
        if (std::cmp_greater(std::count(id.begin(), id.end(), '/'),
                             constants::maxPrefixesInId))
        {
            throw sdbusplus::exception::SdBusError(
                static_cast<int>(std::errc::invalid_argument),
                "Too many prefixes in id");
        }
        return id;
    }
    throw sdbusplus::exception::SdBusError(
        static_cast<int>(std::errc::invalid_argument), "Invalid path prefix");
}
} // namespace utils
