#pragma once

#include <algorithm>

namespace utils
{

template <class R, class T, class... Args,
          template <class, class...> class Container>
auto convContainer(const Container<T, Args...>& container)
{
    Container<R> result;
    std::transform(container.begin(), container.end(),
                   std::back_inserter(result),
                   [](const auto& item) -> R { return item; });
    return result;
}

} // namespace utils
