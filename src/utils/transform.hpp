#pragma once

#include <algorithm>
#include <iterator>

namespace utils
{

template <template <class, class...> class Container, class T, class... Args,
          class F>
auto transform(const Container<T, Args...>& container, F&& functor)
{
    using R = decltype(functor(*container.begin()));

    Container<R> result;
    result.reserve(container.size());
    std::transform(container.begin(), container.end(),
                   std::inserter(result, result.end()),
                   std::forward<F>(functor));
    return result;
}

} // namespace utils
