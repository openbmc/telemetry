#pragma once

#include "interfaces/clock.hpp"
#include "types/duration_types.hpp"

#include <chrono>

class Clock : public interfaces::Clock
{
  public:
    Milliseconds steadyTimestamp() const noexcept override
    {
        return std::chrono::time_point_cast<Milliseconds>(
                   std::chrono::steady_clock::now())
            .time_since_epoch();
    }

    Milliseconds systemTimestamp() const noexcept override
    {
        return std::chrono::time_point_cast<Milliseconds>(
                   std::chrono::system_clock::now())
            .time_since_epoch();
    }
};
