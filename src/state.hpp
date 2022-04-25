#pragma once

#include <bitset>

enum class ReportFlags
{
    enabled,
    valid
};

template <class T>
concept boolean = std::is_same_v<T, bool>;

template <class Flags, class Object, Flags... Keys>
class State
{
  public:
    explicit State(Object& object) : object(object)
    {}

    template <Flags... Indexes>
    void set(boolean auto... values)
    {
        static_assert(sizeof...(Indexes) == sizeof...(values));

        const bool previous = flags.all();

        (flags.set(indexOf<0, Indexes, Keys...>(), values), ...);

        if (previous != flags.all())
        {
            if (flags.all())
            {
                object.activate();
            }
            else
            {
                object.deactivate();
            }
        }
    }

    template <Flags Index>
    bool get() const
    {
        return flags.test(indexOf<0, Index, Keys...>());
    }

    bool isActive() const
    {
        return flags.all();
    }

  private:
    template <size_t Index, Flags flag, Flags it, Flags... list>
    static constexpr size_t indexOf()
    {
        if constexpr (flag == it)
        {
            return Index;
        }
        else
        {
            return indexOf<Index + 1, flag, list...>();
        }
    }

    Object& object;
    std::bitset<sizeof...(Keys)> flags{0};
};
