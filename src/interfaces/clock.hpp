#pragma once

#include "types/duration_types.hpp"

#include <chrono>

namespace interfaces
{

class Clock
{
  public:
    virtual ~Clock() = default;

    virtual Milliseconds steadyTimestamp() const noexcept = 0;
    virtual Milliseconds systemTimestamp() const noexcept = 0;
};

} // namespace interfaces
