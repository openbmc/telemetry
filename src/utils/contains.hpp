#pragma once

#include <algorithm>

namespace utils
{
namespace detail
{

template <class T>
concept HasMemberFind =
    requires(T container) { container.find(container.begin()->first); };

template <class T>
concept HasMemberContains =
    requires(T container) { container.contains(*container.begin()); };

} // namespace detail

template <detail::HasMemberFind T>
inline bool contains(const T& container,
                     const typename T::value_type::first_type& key)
{
    return container.find(key) != container.end();
}

template <detail::HasMemberContains T>
inline bool contains(const T& container, const typename T::value_type& key)
{
    return container.contains(key);
}

template <class T>
inline bool contains(const T& container, const typename T::value_type& key)
{
    return std::find(container.begin(), container.end(), key) !=
           container.end();
}

} // namespace utils
