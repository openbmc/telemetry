#pragma once

#include <type_traits>

template <class T>
class is_streamable
{
    template <typename U>
    static std::true_type
        check(std::decay_t<decltype(std::declval<std::ostream>()
                                    << std::declval<U>())>*);

    template <typename>
    static std::false_type check(...);

  public:
    static constexpr bool value = decltype(check<T>(nullptr))::value;
};

template <class T>
constexpr bool is_streamable_v = is_streamable<T>::value;
