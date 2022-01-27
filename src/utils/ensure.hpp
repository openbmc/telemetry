#pragma once

#include <utility>

namespace utils
{

template <class F>
struct Ensure
{
    Ensure(F&& functor) : functor(std::forward<F>(functor))
    {}

    ~Ensure()
    {
        functor();
    }

  private:
    F functor;
};

} // namespace utils
