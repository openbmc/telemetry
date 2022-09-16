#pragma once

#include <cstdint>
#include <string>

struct MetricValue
{
    std::string id;
    std::string metadata;
    double value;
    uint64_t timestamp;

    MetricValue(std::string_view idIn, std::string_view metadataIn,
                double valueIn, uint64_t timestampIn) :
        id(idIn),
        metadata(metadataIn), value(valueIn), timestamp(timestampIn)
    {}
};
