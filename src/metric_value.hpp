#pragma once

#include <cstdint>
#include <string>

struct MetricValue
{
    std::string a;
    std::string b;
    double value;
    uint64_t timestamp;
};
