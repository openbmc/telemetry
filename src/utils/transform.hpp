#pragma once

#include <algorithm>
#include <iterator>

namespace utils
{
namespace detail
{

template <class T>
concept has_member_reserve = requires(T t)
{
    t.reserve(size_t{});
};

} // namespace detail

template <class R, class Container, class F>
auto transform(const Container& container, F&& functor)
{
    auto result = R{};

    if constexpr (detail::has_member_reserve<decltype(result)>)
    {
        result.reserve(container.size());
    }
    std::transform(container.begin(), container.end(),
                   std::inserter(result, result.end()),
                   std::forward<F>(functor));
    return result;
}

template <template <class, class...> class Container, class T, class... Args,
          class F>
auto transform(const Container<T, Args...>& container, F&& functor)
{
    using R = Container<decltype(functor(*container.begin()))>;
    return transform<R>(container, std::forward<F>(functor));
}

} // namespace utils
