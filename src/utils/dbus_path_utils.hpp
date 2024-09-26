#pragma once

#include "errors.hpp"

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
constexpr size_t maxPrefixLength{TELEMETRY_MAX_PREFIX_LENGTH};
constexpr size_t maxIdNameLength{TELEMETRY_MAX_ID_NAME_LENGTH};
constexpr size_t maxDbusPathLength{TELEMETRY_MAX_DBUS_PATH_LENGTH};

constexpr size_t maxTriggeFullIdLength{maxDbusPathLength -
                                       triggerDirStr.length()};
constexpr size_t maxReportFullIdLength{maxDbusPathLength -
                                       reportDirStr.length()};

static_assert(maxPrefixesInId * (maxPrefixLength + 1) + maxIdNameLength <=
                  maxTriggeFullIdLength,
              "Misconfigured prefix/id/name lengths.");
static_assert(maxPrefixesInId * (maxPrefixLength + 1) + maxIdNameLength <=
                  maxReportFullIdLength,
              "Misconfigured prefix/id/name lengths.");

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

inline void verifyIdCharacters(std::string_view id)
{
    if (id.find_first_not_of(utils::constants::allowedCharactersInPath) !=
        std::string::npos)
    {
        errors::throwInvalidArgument("Id", "Invalid character.");
    }
}

sdbusplus::message::object_path pathAppend(sdbusplus::message::object_path path,
                                           const std::string& appended);

std::string reportPathToId(const sdbusplus::message::object_path& path);

void verifyIdPrefixes(std::string_view id);

} // namespace utils
