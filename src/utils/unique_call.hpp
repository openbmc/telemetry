#pragma once

#include <utility>

namespace utils
{

class UniqueCall
{
  public:
    struct Lock
    {
        explicit Lock(UniqueCall* unique) : unique(unique)
        {
            unique->value = true;
        }

        Lock(const Lock&) = delete;
        Lock(Lock&& other) : unique(other.unique)
        {
            other.unique = nullptr;
        }

        Lock& operator=(const Lock&) = delete;
        Lock& operator=(Lock&& other)
        {
            unlock();

            std::swap(unique, other.unique);

            return *this;
        }

        ~Lock()
        {
            unlock();
        }

      private:
        void unlock()
        {
            if (unique)
            {
                unique->value = false;
            }
            unique = nullptr;
        }
        UniqueCall* unique;
    };

    template <class Functor, class... Args>
    void operator()(Functor&& functor, Args&&... args)
    {
        if (value == false)
        {
            functor(Lock(this), std::forward<Args>(args)...);
        }
    }

  private:
    bool value = false;
};

} // namespace utils
