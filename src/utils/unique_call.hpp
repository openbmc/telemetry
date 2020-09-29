#pragma once

#include <memory>

struct UniqueCall
{
    virtual ~UniqueCall() = default;
};

namespace detail
{

template <class Tag>
struct UniqueCallT : public UniqueCall
{
    UniqueCallT()
    {
        used = true;
    }

    ~UniqueCallT()
    {
        used = false;
    }

    static bool used;
};

template <class Tag>
bool UniqueCallT<Tag>::used = false;

} // namespace detail

template <class Tag, class Functor>
inline void uniqueCall(Functor&& functor)
{
    if (detail::UniqueCallT<Tag>::used == false)
    {
        auto u = std::make_unique<detail::UniqueCallT<Tag>>();
        functor(std::move(u));
    }
}
