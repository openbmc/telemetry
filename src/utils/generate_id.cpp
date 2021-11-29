#include "utils/generate_id.hpp"

#include <sdbusplus/exception.hpp>

#include <system_error>

namespace utils
{
namespace details
{

static constexpr std::string_view allowedCharactersInId =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_/";

}

void verifyIdCharacters(std::string_view triggerId)
{
    if (triggerId.find_first_not_of(details::allowedCharactersInId) !=
        std::string::npos)
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::invalid_argument),
            "Invalid character in id");
    }
}

std::string generateId(std::string_view prefix, std::string_view name,
                       const std::vector<std::string>& conflictIds,
                       size_t maxLength)
{
    verifyIdCharacters(prefix);

    if (!prefix.empty() && !prefix.ends_with('/'))
    {
        return std::string(prefix);
    }

    std::string strippedId(name);
    strippedId.erase(
        std::remove_if(strippedId.begin(), strippedId.end(),
                       [](char c) {
                           return c == '/' ||
                                  details::allowedCharactersInId.find(c) ==
                                      std::string_view::npos;
                       }),
        strippedId.end());
    strippedId = std::string(prefix) + strippedId;

    size_t idx = 0;
    std::string tmpId = strippedId.substr(0, maxLength);

    while (std::find(conflictIds.begin(), conflictIds.end(), tmpId) !=
               conflictIds.end() ||
           tmpId.empty())
    {
        tmpId = strippedId.substr(0, maxLength - std::to_string(idx).length()) +
                std::to_string(idx);
        ++idx;
    }
    return tmpId;
}

} // namespace utils
