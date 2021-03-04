#pragma once

#include <chrono>

namespace interfaces
{

class Clock
{
  public:
    using duration = std::chrono::steady_clock::time_point::duration;
    using rep = std::chrono::steady_clock::time_point::rep;
    using period = std::chrono::steady_clock::time_point::period;
    using time_point = std::chrono::steady_clock::time_point;

    virtual ~Clock() = default;

    virtual time_point now() const noexcept = 0;
    virtual uint64_t timestamp() const noexcept = 0;
};

} // namespace interfaces
