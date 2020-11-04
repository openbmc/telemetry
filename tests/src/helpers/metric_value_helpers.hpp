#pragma once

#include "metric_value.hpp"

#include <gmock/gmock.h>

inline void PrintTo(const MetricValue& o, std::ostream* os)
{
    (*os) << "{ id: " << o.id << ", metadata: " << o.metadata
          << ", value: " << o.value << ", timestamp: " << o.timestamp << " }";
}

inline bool operator==(const MetricValue& left, const MetricValue& right)
{
    return std::tie(left.id, left.metadata, left.value, left.timestamp) ==
           std::tie(right.id, right.metadata, right.value, right.timestamp);
}