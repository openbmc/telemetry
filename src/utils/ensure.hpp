#pragma once

#include <optional>
#include <utility>

namespace utils
{

template <class F>
struct Ensure
{
    Ensure() = default;

    template <class U>
    Ensure(U&& functor) : functor(std::forward<U>(functor))
    {}

    Ensure(F functor) : functor(std::move(functor)) {}

    Ensure(Ensure&& other) = delete;
    Ensure(const Ensure&) = delete;

    ~Ensure()
    {
        call();
    }

    template <class U>
    Ensure& operator=(U&& other)
    {
        call();
        functor = std::move(other);
        return *this;
    }

    Ensure& operator=(Ensure&& other) = delete;

    Ensure& operator=(std::nullptr_t)
    {
        call();
        functor = std::nullopt;
        return *this;
    }

    Ensure& operator=(const Ensure&) = delete;

    explicit operator bool() const
    {
        return static_cast<bool>(functor);
    }

  private:
    void call()
    {
        if (functor)
        {
            (*functor)();
        }
    }

    std::optional<F> functor;
};

} // namespace utils
