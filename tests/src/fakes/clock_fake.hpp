#pragma once

#include "interfaces/clock.hpp"
#include "types/duration_type.hpp"

class ClockFake : public interfaces::Clock
{
  public:
    time_point now() const noexcept override
    {
        return timePoint;
    }

    uint64_t timestamp() const noexcept override
    {
        return toTimestamp(now());
    }

    uint64_t advance(DurationType delta) noexcept
    {
        timePoint += delta;
        return timestamp();
    }

    void set(DurationType timeSinceEpoch) noexcept
    {
        timePoint = time_point{timeSinceEpoch};
    }

    static uint64_t toTimestamp(DurationType time)
    {
        return time.count();
    }

    static uint64_t toTimestamp(time_point tp)
    {
        return std::chrono::time_point_cast<DurationType>(tp)
            .time_since_epoch()
            .count();
    }

  private:
    time_point timePoint = std::chrono::steady_clock::now();
};
