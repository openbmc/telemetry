#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace utils
{

void verifyIdCharacters(std::string_view triggerId);
std::string generateId(std::string_view prefix, std::string_view triggerName,
                       const std::vector<std::string>& conflictIds,
                       size_t maxLength);

} // namespace utils
