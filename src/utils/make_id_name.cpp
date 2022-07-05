#include "utils/make_id_name.hpp"

#include "utils/dbus_path_utils.hpp"

#include <sdbusplus/exception.hpp>

#include <algorithm>
#include <system_error>

namespace utils
{
namespace details
{

size_t countDigits(size_t value)
{
    size_t result = 1;
    while (value >= 10)
    {
        ++result;
        value /= 10;
    }
    return result;
}

std::string generateId(std::string_view idIn, std::string_view nameIn,
                       const std::vector<std::string>& conflictIds,
                       size_t maxFullLength)
{
    verifyIdCharacters(idIn);

    if ((idIn.length() > maxFullLength) ||
        (idIn.ends_with('/') && idIn.length() >= maxFullLength))
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::invalid_argument), "Id too long");
    }

    verifyIdPrefixes(idIn);

    if (!idIn.empty() && !idIn.ends_with('/'))
    {
        if (std::find(conflictIds.begin(), conflictIds.end(), idIn) !=
            conflictIds.end())
        {
            throw sdbusplus::exception::SdBusError(
                static_cast<int>(std::errc::file_exists), "Duplicated id");
        }
        return std::string(idIn);
    }

    const std::string prefixes(idIn);
    size_t maxIdLength = maxFullLength - prefixes.length();
    if constexpr (constants::maxIdNameLength > 0)
    {
        maxIdLength = std::min(maxIdLength, constants::maxIdNameLength);
    }

    std::string strippedId(nameIn);
    strippedId.erase(
        std::remove_if(
            strippedId.begin(), strippedId.end(),
            [](char c) {
                return c == '/' ||
                       utils::constants::allowedCharactersInPath.find(c) ==
                           std::string_view::npos;
            }),
        strippedId.end());

    size_t idx = 0;
    std::string tmpId = prefixes + strippedId.substr(0, maxIdLength);

    while (std::find(conflictIds.begin(), conflictIds.end(), tmpId) !=
               conflictIds.end() ||
           tmpId.empty())
    {
        size_t digitsInIdx = countDigits(idx);

        if (digitsInIdx > maxIdLength)
        {
            throw sdbusplus::exception::SdBusError(
                static_cast<int>(std::errc::file_exists),
                "Unique indices are depleted");
        }

        tmpId = prefixes + strippedId.substr(0, maxIdLength - digitsInIdx) +
                std::to_string(idx);
        ++idx;
    }

    return tmpId;
}

} // namespace details

std::pair<std::string, std::string> makeIdName(
    std::string_view id, std::string_view name, std::string_view defaultName,
    const std::vector<std::string>& conflictIds, const size_t maxLength)
{
    if constexpr (constants::maxIdNameLength > 0)
    {
        if (name.length() > constants::maxIdNameLength)
        {
            throw sdbusplus::exception::SdBusError(
                static_cast<int>(std::errc::invalid_argument), "Name too long");
        }
    }

    if (name.empty() && !id.ends_with('/'))
    {
        name = id;

        if (auto pos = name.find_last_of("/"); pos != std::string::npos)
        {
            name = name.substr(pos + 1);
        }

        if constexpr (constants::maxIdNameLength > 0)
        {
            name = name.substr(0, constants::maxIdNameLength);
        }
    }

    if (name.empty())
    {
        name = defaultName;
    }

    return std::make_pair(details::generateId(id, name, conflictIds, maxLength),
                          std::string{name});
}

} // namespace utils
