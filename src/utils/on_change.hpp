#pragma once

#include <boost/signals2.hpp>

namespace utils
{
template <class T>
class OnChange
{
  public:
    virtual ~OnChange() = default;

    template <class F>
    boost::signals2::scoped_connection onChange(F&& fun)
    {
        return signal_.connect(std::forward<F>(fun));
    }

  protected:
    void changed() const
    {
        signal_(*static_cast<const T*>(this));
    }

  private:
    boost::signals2::signal<void(const T&)> signal_;
};
} // namespace utils
