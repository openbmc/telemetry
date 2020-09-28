#pragma once

#include <sstream>
#include <type_traits>

template <class T>
class is_streamable
{
    template <typename U>
    static std::true_type
        check(std::decay_t<decltype(std::declval<std::ostringstream>()
                                    << std::declval<U>())>*);

    template <typename>
    static std::false_type check(...);

  public:
    static constexpr bool value = decltype(check<T>(nullptr))::value;
};

template <class T>
constexpr bool is_streamable_v = is_streamable<T>::value;

static_assert(is_streamable_v<char[5]>, "is_streamable failed to verify char*");
static_assert(is_streamable_v<double>, "is_streamable failed to verify double");
