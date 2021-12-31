#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace utils
{

void verifyIdCharacters(std::string_view id);
std::pair<std::string, std::string> generateId(
    std::string_view id, std::string_view name, std::string_view defaultName,
    const std::vector<std::string>& conflictIds, const size_t maxLength);

} // namespace utils
