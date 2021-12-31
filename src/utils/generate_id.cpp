#include "utils/generate_id.hpp"

#include <sdbusplus/exception.hpp>

#include <system_error>

namespace utils
{
namespace details
{

static constexpr std::string_view allowedCharactersInId =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_/";

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

std::string generateId(std::string_view id, std::string_view name,
                       const std::vector<std::string>& conflictIds,
                       size_t maxLength)
{
    verifyIdCharacters(id);

    if (id.starts_with('/'))
    {
        id = id.substr(1);
    }

    if ((id.length() > maxLength) ||
        (id.ends_with('/') && id.length() >= maxLength))
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::invalid_argument), "Id too long");
    }

    if (!id.empty() && !id.ends_with('/'))
    {
        if (std::find(conflictIds.begin(), conflictIds.end(), id) !=
            conflictIds.end())
        {
            throw sdbusplus::exception::SdBusError(
                static_cast<int>(std::errc::file_exists), "Duplicated id");
        }
        return std::string(id);
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
    strippedId = std::string(id) + strippedId;

    size_t idx = 0;
    std::string tmpId = strippedId.substr(0, maxLength);

    while (std::find(conflictIds.begin(), conflictIds.end(), tmpId) !=
               conflictIds.end() ||
           tmpId.empty())
    {
        size_t digitsInIdx = countDigits(idx);

        if (digitsInIdx > maxLength)
        {
            throw sdbusplus::exception::SdBusError(
                static_cast<int>(std::errc::file_exists),
                "Unique indices are depleted");
        }

        tmpId =
            strippedId.substr(0, maxLength - digitsInIdx) + std::to_string(idx);
        ++idx;
    }

    return tmpId;
}

} // namespace details

void verifyIdCharacters(std::string_view id)
{
    if (id.find_first_not_of(details::allowedCharactersInId) !=
        std::string::npos)
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::invalid_argument),
            "Invalid character in id");
    }

    if (auto pos = id.find_first_of("/");
        pos != std::string::npos && pos != id.find_last_of("/"))
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::invalid_argument),
            "Too many '/' in id");
    }
}

std::pair<std::string, std::string> generateId(
    std::string_view id, std::string_view name, std::string_view defaultName,
    const std::vector<std::string>& conflictIds, const size_t maxLength)
{
    if (name.empty() && !id.ends_with('/'))
    {
        name = id;

        if (auto pos = name.find_last_of("/"); pos != std::string::npos)
        {
            name = name.substr(pos + 1);
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
