#pragma once

#include <cstdint>
#include <string>

struct MetricValue
{
    std::string metadata;
    double value;
    uint64_t timestamp;

    MetricValue(std::string_view metadataIn, double valueIn,
                uint64_t timestampIn) :
        metadata(metadataIn),
        value(valueIn), timestamp(timestampIn)
    {}
};
