#pragma once

#include <memory>
#include <utility>

namespace utils
{

class UniqueCall
{
  public:
    struct Lock
    {};

    template <class Functor, class... Args>
    void operator()(Functor&& functor, Args&&... args)
    {
        if (lock.expired())
        {
            auto l = std::make_shared<Lock>();
            lock = l;
            functor(std::move(l), std::forward<Args>(args)...);
        }
    }

  private:
    std::weak_ptr<Lock> lock;
};

} // namespace utils
