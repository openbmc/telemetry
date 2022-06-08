#pragma once

#include <sdbusplus/message.hpp>

#include <algorithm>
#include <ranges>
#include <string_view>

namespace utils
{

namespace constants
{
constexpr std::string_view triggerDirStr =
    "/xyz/openbmc_project/Telemetry/Triggers/";
constexpr std::string_view reportDirStr =
    "/xyz/openbmc_project/Telemetry/Reports/";

constexpr std::string_view allowedCharactersInPath =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_/";
constexpr size_t maxPrefixesInId = 1;

const sdbusplus::message::object_path triggerDirPath =
    sdbusplus::message::object_path(std::string(triggerDirStr));
const sdbusplus::message::object_path reportDirPath =
    sdbusplus::message::object_path(std::string(reportDirStr));
} // namespace constants

inline bool isValidDbusPath(const std::string& path)
{
    return (path.find_first_not_of(constants::allowedCharactersInPath) ==
            std::string::npos) &&
           !path.ends_with('/');
}

sdbusplus::message::object_path pathAppend(sdbusplus::message::object_path path,
                                           const std::string& appended);

std::string reportPathToId(const sdbusplus::message::object_path& path);

} // namespace utils
