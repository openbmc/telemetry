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
                       std::string_view defaultName,
                       const std::vector<std::string>& conflictIds)
{
    verifyIdCharacters(idIn);
    verifyIdPrefixes(idIn);

    if (!idIn.empty() && !idIn.ends_with('/'))
    {
        if (std::find(conflictIds.begin(), conflictIds.end(), idIn) !=
            conflictIds.end())
        {
            errors::throwNotAllowed("Duplicated id");
        }
        return std::string(idIn);
    }

    const std::string prefixes(idIn);

    std::string strippedId(nameIn);
    if (strippedId.find_first_of(utils::constants::allowedCharactersInPath) ==
        std::string::npos)
    {
        strippedId = defaultName;
    }
    strippedId.erase(std::remove_if(strippedId.begin(), strippedId.end(),
                                    [](char c) {
        return c == '/' || utils::constants::allowedCharactersInPath.find(c) ==
                               std::string_view::npos;
    }),
                     strippedId.end());

    size_t idx = 0;
    std::string tmpId = prefixes +
                        strippedId.substr(0, constants::maxIdNameLength);

    while (std::find(conflictIds.begin(), conflictIds.end(), tmpId) !=
           conflictIds.end())
    {
        size_t digitsInIdx = countDigits(idx);

        if (digitsInIdx > constants::maxIdNameLength)
        {
            errors::throwNotAllowed("Unique indices are depleted");
        }

        tmpId = prefixes +
                strippedId.substr(0, constants::maxIdNameLength - digitsInIdx) +
                std::to_string(idx);
        ++idx;
    }

    return tmpId;
}

} // namespace details

std::pair<std::string, std::string>
    makeIdName(std::string_view id, std::string_view name,
               std::string_view defaultName,
               const std::vector<std::string>& conflictIds)
{
    if (name.length() > constants::maxIdNameLength)
    {
        errors::throwInvalidArgument("Name", "Too long.");
    }

    if (name.empty() && !id.ends_with('/'))
    {
        name = id;

        if (auto pos = name.find_last_of("/"); pos != std::string::npos)
        {
            name = name.substr(pos + 1);
        }

        name = name.substr(0, constants::maxIdNameLength);
    }

    if (name.empty())
    {
        name = defaultName;
    }

    return std::make_pair(
        details::generateId(id, name, defaultName, conflictIds),
        std::string{name});
}

} // namespace utils
