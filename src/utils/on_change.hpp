#pragma once

#include "signals.hpp"

namespace utils
{

template <class T>
class OnChange
{
  public:
    virtual ~OnChange() = default;

    template <class F>
    signals::Connection onChange(F&& fun)
    {
        return signal_.connect(std::forward<F>(fun));
    }

  protected:
    void changed() const
    {
        signal_(*static_cast<const T*>(this));
    }

  private:
    signals::Signal<void(const T&)> signal_;
};

} // namespace utils
