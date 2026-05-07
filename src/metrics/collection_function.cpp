#include "metrics/collection_function.hpp"

#include <cmath>

namespace metrics
{

class FunctionMinimum : public CollectionFunction
{
  public:
    double calculate(const StreamingStats& stats, Milliseconds) const override
    {
        return stats.getMin();
    }
};

class FunctionMaximum : public CollectionFunction
{
  public:
    double calculate(const StreamingStats& stats, Milliseconds) const override
    {
        return stats.getMax();
    }
};

class FunctionAverage : public CollectionFunction
{
  public:
    double calculate(const StreamingStats& stats,
                     Milliseconds timestamp) const override
    {
        // Time-weighted average (value * milliseconds / total_milliseconds)
        if (stats.count == 0)
        {
            return 0.0;
        }

        // Calculate total time-weighted sum including the last reading
        double totalTimeWeightedSum = stats.getTimeWeightedSumMs();
        Milliseconds totalDuration = stats.getTotalDuration();

        // Add contribution from last reading to current timestamp
        if (timestamp > stats.lastTimestamp)
        {
            Milliseconds lastDuration = timestamp - stats.lastTimestamp;
            totalTimeWeightedSum += stats.lastValue * lastDuration.count();
            totalDuration += lastDuration;
        }

        // Return time-weighted average
        return totalTimeWeightedSum /
               std::max(totalDuration.count(), uint64_t{1u});
    }
};

class FunctionSummation : public CollectionFunction
{
    using Multiplier = std::chrono::duration<double>;

  public:
    double calculate(const StreamingStats& stats,
                     Milliseconds timestamp) const override
    {
        // Time-weighted sum (integral) - value * seconds
        if (stats.count == 0)
        {
            return 0.0;
        }

        // Get accumulated time-weighted sum (already in seconds)
        double totalTimeWeightedSum = stats.getTimeWeightedSumSec();

        // Add contribution from last reading to current timestamp
        if (timestamp > stats.lastTimestamp)
        {
            Milliseconds lastDuration = timestamp - stats.lastTimestamp;
            const auto multiplier = calculateMultiplier(lastDuration);
            totalTimeWeightedSum += stats.lastValue * multiplier.count();
        }

        return totalTimeWeightedSum;
    }

  private:
    static constexpr Multiplier calculateMultiplier(Milliseconds duration)
    {
        constexpr auto m = Multiplier{Seconds{1}};
        return Multiplier{duration / m};
    }
};

std::shared_ptr<CollectionFunction> makeCollectionFunction(
    OperationType operationType)
{
    using namespace std::string_literals;

    switch (operationType)
    {
        case OperationType::min:
            return std::make_shared<FunctionMinimum>();
        case OperationType::max:
            return std::make_shared<FunctionMaximum>();
        case OperationType::avg:
            return std::make_shared<FunctionAverage>();
        case OperationType::sum:
            return std::make_shared<FunctionSummation>();
        default:
            throw std::runtime_error(
                "op: "s + utils::enumToString(operationType) +
                " is not supported"s);
    }
}

} // namespace metrics
