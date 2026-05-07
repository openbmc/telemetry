#pragma once

#include "types/duration_types.hpp"
#include "types/operation_type.hpp"

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

    void reset()
    {
        sum = 0.0;
        min = std::numeric_limits<double>::max();
        max = std::numeric_limits<double>::lowest();
        count = 0;
        firstTimestamp = Milliseconds{0};
        lastTimestamp = Milliseconds{0};
        lastValue = 0.0;
    }

    void addReading(Milliseconds timestamp, double value)
    {
        if (count == 0)
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
};

class CollectionFunction
{
  public:
    virtual ~CollectionFunction() = default;

    // Original interface for backward compatibility
    virtual double calculate(const std::vector<ReadingItem>& readings,
                             Milliseconds timestamp) const = 0;
    virtual double calculateForStartupInterval(
        std::vector<ReadingItem>& readings, Milliseconds timestamp) const = 0;

    // New streaming interface
    virtual double calculateStreaming(const StreamingStats& stats,
                                      Milliseconds timestamp) const = 0;
};

std::shared_ptr<CollectionFunction> makeCollectionFunction(OperationType);

} // namespace metrics
