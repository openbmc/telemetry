#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace utils
{

std::pair<std::string, std::string>
    makeIdName(std::string_view id, std::string_view name,
               std::string_view defaultName,
               const std::vector<std::string>& conflictIds);

} // namespace utils
