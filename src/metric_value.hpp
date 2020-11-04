#pragma once

#include <cstdint>
#include <string>

struct MetricValue
{
    std::string id;
    std::string metadata;
    double value;
    uint64_t timestamp;
};
