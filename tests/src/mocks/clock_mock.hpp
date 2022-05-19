#pragma once

#include "interfaces/clock.hpp"
#include "types/duration_types.hpp"

#include <gmock/gmock.h>

class ClockMock : public interfaces::Clock
{
  public:
    MOCK_METHOD(Milliseconds, steadyTimestamp, (), (const, noexcept, override));
    MOCK_METHOD(Milliseconds, systemTimestamp, (), (const, noexcept, override));
};
