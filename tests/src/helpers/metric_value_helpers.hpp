#pragma once

#include "metric_value.hpp"

#include <gmock/gmock.h>

inline void PrintTo(const MetricValue& o, std::ostream* os)
{
    (*os) << "{ metadata: " << o.metadata << ", value: " << o.value
          << ", timestamp: " << o.timestamp << " }";
}

inline bool operator==(const MetricValue& left, const MetricValue& right)
{
    return std::tie(left.metadata, left.value, left.timestamp) ==
           std::tie(right.metadata, right.value, right.timestamp);
}
