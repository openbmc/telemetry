#include "utils/string_utils.hpp"

#include "utils/dbus_path_utils.hpp"

#include <cmath>

namespace details
{
constexpr std::string_view allowedCharactersInId =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";

std::string repeat(size_t n)
{
    std::string result;
    for (size_t i = 0; i < n; i++)
    {
        result += allowedCharactersInId;
    }
    return result;
}

std::string getString(size_t length)
{
    return details::repeat(
               std::ceil(static_cast<double>(length) /
                         static_cast<double>(allowedCharactersInId.length())))
        .substr(0, length);
}

std::string getStringWithSpaces(size_t length)
{
    std::string result = getString(length);
    size_t idx = 1;
    while (idx < length)
    {
        result[idx] = ' ';
        idx += 5;
    }
    return result;
}
} // namespace details

namespace utils::string_utils
{
std::string getMaxPrefix()
{
    return details::getString(constants::maxPrefixLength);
}

std::string getMaxId()
{
    return details::getString(constants::maxIdNameLength);
}

std::string getMaxName()
{
    return details::getStringWithSpaces(constants::maxIdNameLength);
}

std::string getTooLongPrefix()
{
    return details::getString(constants::maxPrefixLength + 1);
}

std::string getTooLongId()
{
    return details::getString(constants::maxIdNameLength + 1);
}

std::string getTooLongName()
{
    return details::getStringWithSpaces(constants::maxIdNameLength + 1);
}
} // namespace utils::string_utils