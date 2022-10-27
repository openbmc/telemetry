#pragma once

#include <string_view>

namespace utils
{

inline std::string_view toShortEnum(std::string_view name)
{
    auto pos = name.find_last_of(".");
    if (pos != std::string_view::npos && pos + 1 < name.size())
    {
        return name.substr(pos + 1);
    }
    return name;
}

} // namespace utils
