#pragma once

#include "utils/transform.hpp"

namespace utils
{

template <class R, class Container>
auto convContainer(const Container& container)
{
    return transform(container, [](const auto& item) -> R { return item; });
}

// template <class R, class Container>
// auto convContainer(Container&& container)
//{
//    std::vector<R> result;
//    result.reserve(container.size());
//    for (auto& item : container)
//    {
//        result.emplace_back(std::move(item));
//    }
//    container.clear();
//    return result;
//}

} // namespace utils
