#pragma once

#include <algorithm>
#include <vector>

namespace utils
{

template <class T, class U>
std::vector<T> transformTo(const std::vector<U>& input)
{
    std::vector<T> result;
    result.reserve(input.size());
    std::transform(input.begin(), input.end(), std::back_inserter(result),
                   [](const U& item) -> T { return item; });
    return result;
}

} // namespace utils
