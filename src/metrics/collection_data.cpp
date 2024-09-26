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
            errors::throwInvalidArgument(
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
            readings.front().first = std::max(readings.front().first,
                                              timestamp - duration.t);
        }
    }

    std::shared_ptr<CollectionFunction> function;
    std::vector<ReadingItem> readings;
    CollectionDuration duration;
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

std::vector<std::unique_ptr<CollectionData>>
    makeCollectionData(size_t size, OperationType op,
                       CollectionTimeScope timeScope,
                       CollectionDuration duration)
{
    using namespace std::string_literals;

    std::vector<std::unique_ptr<CollectionData>> result;

    result.reserve(size);

    switch (timeScope)
    {
        case CollectionTimeScope::interval:
            std::generate_n(std::back_inserter(result), size,
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
