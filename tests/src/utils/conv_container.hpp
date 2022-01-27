#pragma once

#include <algorithm>

namespace utils
{

template <class R, class T, class... Args,
          template <class, class...> class Container>
auto convContainer(Container<T, Args...>& container)
{
    Container<R> result;
    std::transform(container.begin(), container.end(),
                   std::back_inserter(result), [](auto& item) -> R {
                       if constexpr (std::is_copy_constructible_v<
                                         std::decay_t<decltype(item)>>)
                       {
                           return item;
                       }
                       else
                       {
                           return std::move(item);
                       }
                   });
    return result;
}

} // namespace utils
