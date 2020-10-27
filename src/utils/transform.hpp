#pragma once

#include <algorithm>
#include <iterator>

namespace utils
{
namespace detail
{

template <class T>
struct has_member_reserve
{
    template <class U>
    static U& ref();

    template <class U>
    static std::true_type check(decltype(ref<U>().reserve(size_t{}))*);

    template <class>
    static std::false_type check(...);

    static constexpr bool value =
        decltype(check<std::decay_t<T>>(nullptr))::value;
};

template <class T>
constexpr bool has_member_reserve_v = has_member_reserve<T>::value;

} // namespace detail

template <template <class, class...> class Container, class T, class... Args,
          class F>
auto transform(const Container<T, Args...>& container, F&& functor)
{
    using R = decltype(functor(*container.begin()));

    Container<R> result;
    if constexpr (detail::has_member_reserve_v<Container<T, Args...>>)
    {
        result.reserve(container.size());
    }
    std::transform(container.begin(), container.end(),
                   std::inserter(result, result.end()),
                   std::forward<F>(functor));
    return result;
}

} // namespace utils
