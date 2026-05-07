#pragma once

#include "types/duration_types.hpp"
#include "types/operation_type.hpp"

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <utility>
#include <vector>

namespace metrics
{

using ReadingItem = std::pair<Milliseconds, double>;

struct StreamingStats
{
    double sum = 0.0;
    double min = std::numeric_limits<double>::max();
    double max = std::numeric_limits<double>::lowest();
    uint64_t count = 0;
    Milliseconds firstTimestamp{0};
    Milliseconds lastTimestamp{0};
    double lastValue = 0.0;
    double timeWeightedSumMs = 0.0;  // For average (value * milliseconds)
    double timeWeightedSumSec = 0.0; // For sum (value * seconds)
    Milliseconds totalDuration{0};   // Total time duration

    void reset()
    {
        sum = 0.0;
        min = std::numeric_limits<double>::max();
        max = std::numeric_limits<double>::lowest();
        count = 0;
        firstTimestamp = Milliseconds{0};
        lastTimestamp = Milliseconds{0};
        lastValue = 0.0;
        timeWeightedSumMs = 0.0;
        timeWeightedSumSec = 0.0;
        totalDuration = Milliseconds{0};
    }

    void addReading(Milliseconds timestamp, double value)
    {
        // Skip NaN values (match original behavior)
        if (!std::isfinite(value))
        {
            return;
        }

        // If we have a previous reading, calculate time-weighted contribution
        if (count > 0)
        {
            Milliseconds duration = timestamp - lastTimestamp;
            // For average: value * milliseconds
            timeWeightedSumMs += lastValue * duration.count();
            // For sum: value * seconds
            timeWeightedSumSec += lastValue * (duration.count() / 1000.0);
            totalDuration += duration;
        }
        else
        {
            firstTimestamp = timestamp;
        }

        sum += value;
        count++;
        min = std::min(min, value);
        max = std::max(max, value);
        lastTimestamp = timestamp;
        lastValue = value;
    }

    double getMin() const
    {
        return (count > 0) ? min : 0.0;
    }

    double getMax() const
    {
        return (count > 0) ? max : 0.0;
    }

    double getSum() const
    {
        return sum;
    }

    double getTimeWeightedSumMs() const
    {
        return timeWeightedSumMs;
    }

    double getTimeWeightedSumSec() const
    {
        return timeWeightedSumSec;
    }

    Milliseconds getTotalDuration() const
    {
        return totalDuration;
    }
};

class CollectionFunction
{
  public:
    virtual ~CollectionFunction() = default;

    // Streaming interface - used by all collection types now
    // Provides O(1) memory usage for Interval and Startup collections
    virtual double calculate(const StreamingStats& stats,
                             Milliseconds timestamp) const = 0;
};

std::shared_ptr<CollectionFunction> makeCollectionFunction(OperationType);

} // namespace metrics
