#pragma once

#include "interfaces/sensor.hpp"

#include <gmock/gmock.h>

namespace interfaces
{

inline void PrintTo(const Sensor::Id& o, std::ostream* os)
{
    (*os) << "{ type: " << o.type << ", service: " << o.service
          << ", path: " << o.path << " }";
}

inline bool operator==(const Sensor::Id& left, const Sensor::Id& right)
{
    return std::tie(left.type, left.service, left.path) ==
           std::tie(right.type, right.service, right.path);
}

} // namespace interfaces