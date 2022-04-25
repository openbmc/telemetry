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

    Ensure(F functor) : functor(std::move(functor))
    {}

    Ensure(Ensure&& other) : functor(std::move(other.functor))
    {
        other.functor = std::nullopt;
    }

    Ensure(const Ensure&) = delete;

    ~Ensure()
    {
        clear();
    }

    template <class U>
    Ensure& operator=(U&& other)
    {
        clear();
        functor = std::move(other);
        return *this;
    }

    Ensure& operator=(Ensure&& other)
    {
        clear();
        std::swap(functor, other.functor);
        return *this;
    }

    Ensure& operator=(std::nullptr_t)
    {
        clear();
        return *this;
    }

    Ensure& operator=(const Ensure&) = delete;

    explicit operator bool() const
    {
        return static_cast<bool>(functor);
    }

  private:
    void clear()
    {
        if (functor)
        {
            (*functor)();
            functor = std::nullopt;
        }
    }

    std::optional<F> functor;
};

} // namespace utils
