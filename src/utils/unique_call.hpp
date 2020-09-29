#pragma once

#include <atomic>
#include <memory>
#include <utility>

namespace utils
{

class UniqueCall
{
  public:
    struct Lock
    {
        explicit Lock(UniqueCall& unique) : unique(unique)
        {
            unique.value = true;
        }

        Lock(const Lock&) = delete;
        Lock(Lock&& other) = delete;

        Lock& operator=(const Lock&) = delete;
        Lock& operator=(Lock&& other) = delete;

        ~Lock()
        {
            unique.value = false;
        }

      private:
        UniqueCall& unique;
    };

    template <class Functor, class... Args>
    void operator()(Functor&& functor, Args&&... args)
    {
        if (value == false)
        {
            functor(std::make_shared<Lock>(*this), std::forward<Args>(args)...);
        }
    }

  private:
    std::atomic<bool> value = false;
};

} // namespace utils
