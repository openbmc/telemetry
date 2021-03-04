#pragma once

#include "interfaces/clock.hpp"
#include "types/duration_type.hpp"

#include <chrono>

class Clock : public interfaces::Clock
{
  public:
    time_point now() const noexcept override
    {
        return std::chrono::steady_clock::now();
    }

    uint64_t timestamp() const noexcept override
    {
        return std::chrono::time_point_cast<Milliseconds>(now())
            .time_since_epoch()
            .count();
    }
};
