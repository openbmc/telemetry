#include "metrics/collection_data.hpp"

#include "metrics/collection_function.hpp"

namespace metrics
{

bool CollectionData::updateLastValue(double value)
{
    const bool changed = lastValue != value;
    lastValue = value;
    return changed;
}

class DataPoint : public CollectionData
{
  public:
    std::optional<double> update(Milliseconds) override
    {
        return lastReading;
    }

    double update(Milliseconds, double reading) override
    {
        lastReading = reading;
        return reading;
    }

  private:
    std::optional<double> lastReading;
};

class DataInterval : public CollectionData
{
  public:
    DataInterval(std::shared_ptr<CollectionFunction> function,
                 CollectionDuration duration) :
        function(std::move(function)), duration(duration)
    {
        if (duration.t.count() == 0)
        {
            throw errors::InvalidArgument(
                "ReadingParameters.CollectionDuration");
        }
    }

    std::optional<double> update(Milliseconds timestamp) override
    {
        if (readings.empty())
        {
            return std::nullopt;
        }

        cleanup(timestamp);

        return function->calculate(readings, timestamp);
    }

    double update(Milliseconds timestamp, double reading) override
    {
        readings.emplace_back(timestamp, reading);

        cleanup(timestamp);

        return function->calculate(readings, timestamp);
    }

  private:
    void cleanup(Milliseconds timestamp)
    {
        auto it = readings.begin();
        for (auto kt = std::next(readings.rbegin()); kt != readings.rend();
             ++kt)
        {
            const auto& [nextItemTimestamp, nextItemReading] = *std::prev(kt);
            if (timestamp >= nextItemTimestamp &&
                timestamp - nextItemTimestamp > duration.t)
            {
                it = kt.base();
                break;
            }
        }
        readings.erase(readings.begin(), it);

        if (timestamp > duration.t)
        {
            readings.front().first =
                std::max(readings.front().first, timestamp - duration.t);
        }
    }

    std::shared_ptr<CollectionFunction> function;
    std::vector<ReadingItem> readings;
    CollectionDuration duration;
};

/**
 * Streaming version of DataInterval - uses O(1) memory with fixed intervals
 * This class uses pure streaming statistics without storing readings.
 */
class DataIntervalStreaming : public CollectionData
{
  public:
    DataIntervalStreaming(std::shared_ptr<CollectionFunction> function,
                          CollectionDuration duration) :
        function(std::move(function)), duration(duration),
        intervalStart(Milliseconds{0})
    {
        if (duration.t.count() == 0)
        {
            throw errors::InvalidArgument(
                "ReadingParameters.CollectionDuration");
        }
    }

    std::optional<double> update(Milliseconds timestamp) override
    {
        if (stats.count == 0)
        {
            return std::nullopt;
        }

        // Check if we need to start a new interval
        checkIntervalBoundary(timestamp);

        return function->calculateStreaming(stats, timestamp);
    }

    double update(Milliseconds timestamp, double reading) override
    {
        // Initialize interval start on first reading
        if (stats.count == 0 && intervalStart.count() == 0)
        {
            intervalStart = timestamp;
        }

        // Check if we need to start a new interval
        checkIntervalBoundary(timestamp);

        // Add reading to current interval's streaming stats
        stats.addReading(timestamp, reading);

        return function->calculateStreaming(stats, timestamp);
    }

  private:
    void checkIntervalBoundary(Milliseconds timestamp)
    {
        // If we've exceeded the duration, start a new interval
        if (intervalStart.count() > 0 &&
            timestamp >= intervalStart + duration.t)
        {
            // Reset stats for new interval
            stats.reset();
            intervalStart = timestamp;
        }
    }

    std::shared_ptr<CollectionFunction> function;
    StreamingStats stats;        // O(1) memory - only current interval stats
    CollectionDuration duration; // Interval duration
    Milliseconds intervalStart;  // Start of current interval
};

class DataStartup : public CollectionData
{
  public:
    explicit DataStartup(std::shared_ptr<CollectionFunction> function) :
        function(std::move(function))
    {}

    std::optional<double> update(Milliseconds timestamp) override
    {
        if (readings.empty())
        {
            return std::nullopt;
        }

        return function->calculateForStartupInterval(readings, timestamp);
    }

    double update(Milliseconds timestamp, double reading) override
    {
        readings.emplace_back(timestamp, reading);
        return function->calculateForStartupInterval(readings, timestamp);
    }

  private:
    std::shared_ptr<CollectionFunction> function;
    std::vector<ReadingItem> readings;
};

std::vector<std::unique_ptr<CollectionData>> makeCollectionData(
    size_t size, OperationType op, CollectionTimeScope timeScope,
    CollectionDuration duration)
{
    using namespace std::string_literals;

    std::vector<std::unique_ptr<CollectionData>> result;

    result.reserve(size);

    switch (timeScope)
    {
        case CollectionTimeScope::interval:
            // Use streaming version for memory efficiency
            std::generate_n(std::back_inserter(result), size,
                            [cf = makeCollectionFunction(op), duration] {
                                return std::make_unique<DataIntervalStreaming>(
                                    cf, duration);
                            });
            break;
        case CollectionTimeScope::point:
            std::generate_n(std::back_inserter(result), size,
                            [] { return std::make_unique<DataPoint>(); });
            break;
        case CollectionTimeScope::startup:
            std::generate_n(std::back_inserter(result), size,
                            [cf = makeCollectionFunction(op)] {
                                return std::make_unique<DataStartup>(cf);
                            });
            break;
    }

    return result;
}

} // namespace metrics
