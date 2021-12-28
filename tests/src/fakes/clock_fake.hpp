#pragma once

#include "interfaces/clock.hpp"
#include "types/duration_types.hpp"

class ClockFake : public interfaces::Clock
{
  public:
    template <class ClockType>
    struct InternalClock
    {
        using clock = ClockType;
        using time_point = typename clock::time_point;

        Milliseconds timestamp() const noexcept
        {
            return ClockFake::toTimestamp(now);
        }

        void advance(Milliseconds delta) noexcept
        {
            now += delta;
        }

        void set(Milliseconds timeSinceEpoch) noexcept
        {
            now = time_point{timeSinceEpoch};
        }

        void reset() noexcept
        {
            now = time_point{Milliseconds{0u}};
        }

      private:
        time_point now = clock::now();
    };

    template <class TimePoint>
    static Milliseconds toTimestamp(TimePoint tp)
    {
        return std::chrono::time_point_cast<Milliseconds>(tp)
            .time_since_epoch();
    }

    Milliseconds steadyTimestamp() const noexcept override
    {
        return steady.timestamp();
    }

    Milliseconds systemTimestamp() const noexcept override
    {
        return system.timestamp();
    }

    void advance(Milliseconds delta) noexcept
    {
        steady.advance(delta);
        system.advance(delta);
    }

    InternalClock<std::chrono::steady_clock> steady;
    InternalClock<std::chrono::system_clock> system;
};
