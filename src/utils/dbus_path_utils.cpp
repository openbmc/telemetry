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
        verifyIdPrefixes(id);
        return id;
    }
    throw sdbusplus::exception::SdBusError(
        static_cast<int>(std::errc::invalid_argument), "Invalid path prefix");
}

void verifyIdPrefixes(std::string_view id)
{
    size_t pos_start = 0;
    size_t pos_end = 0;
    size_t prefix_cnt = 0;
    while ((pos_end = id.find('/', pos_start)) != std::string::npos)
    {
        if (pos_start == pos_end)
        {
            throw errors::InvalidArgument("Id", "Invalid prefixes in id.");
        }

        if (++prefix_cnt > constants::maxPrefixesInId)
        {
            throw errors::InvalidArgument("Id", "Too many prefixes.");
        }

        if (pos_end - pos_start > constants::maxPrefixLength)
        {
            throw errors::InvalidArgument("Id", "Prefix too long.");
        }

        pos_start = pos_end + 1;
    }

    if (id.length() - pos_start > constants::maxIdNameLength)
    {
        throw errors::InvalidArgument("Id", "Too long.");
    }
}
} // namespace utils
