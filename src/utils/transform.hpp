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

template <template <class, class...> class R, class Container, class Functor>
inline auto transform(const Container& container, Functor&& f)
{
    auto result = R<decltype(f(*container.begin()))>{};

    if constexpr (detail::has_member_reserve<decltype(result)>)
    {
        result.reserve(container.size());
    }

    std::transform(container.begin(), container.end(),
                   std::inserter(result, result.end()),
                   std::forward<Functor>(f));

    return result;
}

template <template <class, class...> class Container, class Functor,
          class... Args>
inline auto transform(const Container<Args...>& container, Functor&& f)
{
    return transform<Container, Container<Args...>, Functor>(
        container, std::forward<Functor>(f));
}

} // namespace utils
