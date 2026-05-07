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

        return function->calculate(stats, timestamp);
    }

    double update(Milliseconds timestamp, double reading) override
    {
        if (stats.count == 0)
        {
            intervalStart = timestamp;
        }

        if (intervalStart.count() > 0 &&
            timestamp >= intervalStart + duration.t)
        {
            stats.reset();
            intervalStart = timestamp;
        }

        stats.addReading(timestamp, reading);

        return function->calculate(stats, timestamp);
    }

    std::shared_ptr<CollectionFunction> function;
    StreamingStats stats;
    CollectionDuration duration;
    Milliseconds intervalStart;
};

class DataStartup : public CollectionData
{
  public:
    explicit DataStartup(std::shared_ptr<CollectionFunction> function) :
        function(std::move(function))
    {}

    std::optional<double> update(Milliseconds timestamp) override
    {
        if (stats.count == 0)
        {
            return std::nullopt;
        }

        return function->calculate(stats, timestamp);
    }

    double update(Milliseconds timestamp, double reading) override
    {
        stats.addReading(timestamp, reading);

        return function->calculate(stats, timestamp);
    }

  private:
    std::shared_ptr<CollectionFunction> function;
    StreamingStats stats;
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
            std::generate_n(
                std::back_inserter(result), size,
                [cf = makeCollectionFunction(op), duration] {
                    return std::make_unique<DataInterval>(cf, duration);
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
