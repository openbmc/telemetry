#pragma once

#include "types/trigger_types.hpp"

#include <ostream>

#include <gmock/gmock.h>

inline void PrintTo(const TriggerAction& action, std::ostream* os)
{
    *os << actionToString(action);
}
