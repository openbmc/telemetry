#pragma once

#include "interfaces/clock.hpp"

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

    uint64_t advance(std::chrono::milliseconds delta) noexcept
    {
        timePoint += delta;
        return timestamp();
    }

    void set(std::chrono::milliseconds timeSinceEpoch) noexcept
    {
        timePoint = time_point{timeSinceEpoch};
    }

    static uint64_t toTimestamp(std::chrono::milliseconds time)
    {
        return time.count();
    }

    static uint64_t toTimestamp(time_point tp)
    {
        return std::chrono::time_point_cast<std::chrono::milliseconds>(tp)
            .time_since_epoch()
            .count();
    }

  private:
    time_point timePoint = std::chrono::steady_clock::now();
};
