#include "metric.hpp"

#include "types/types.hpp"
#include "utils/transform.hpp"

#include <algorithm>
#include <iostream>

using ReadingItem = std::pair<uint64_t, double>;

class Metric::CollectionFunction
{
  public:
    virtual ~CollectionFunction() = default;

    virtual ReadingItem calculate(const std::vector<ReadingItem>& readings,
                                  uint64_t timestamp) const = 0;
};

class Metric::FunctionSingle : public Metric::CollectionFunction
{
  public:
    ReadingItem calculate(const std::vector<ReadingItem>& readings,
                          uint64_t) const override
    {
        return readings.back();
    }
};

class Metric::FunctionMinimum : public Metric::CollectionFunction
{
  public:
    ReadingItem calculate(const std::vector<ReadingItem>& readings,
                          uint64_t) const override
    {
        return *std::min_element(readings.begin(), readings.end(),
                                 [](const auto& left, const auto& right) {
                                     return left.second < right.second;
                                 });
    }
};

class Metric::FunctionMaximum : public Metric::CollectionFunction
{
  public:
    ReadingItem calculate(const std::vector<ReadingItem>& readings,
                          uint64_t) const override
    {
        return *std::max_element(readings.begin(), readings.end(),
                                 [](const auto& left, const auto& right) {
                                     return left.second < right.second;
                                 });
    }
};

class Metric::FunctionAverage : public Metric::CollectionFunction
{
  public:
    ReadingItem calculate(const std::vector<ReadingItem>& readings,
                          uint64_t timestamp) const override
    {
        auto valueSum = 0.;
        auto timeSum = uint64_t{0};
        for (auto it = readings.begin(); it != std::prev(readings.end()); ++it)
        {
            const auto kt = std::next(it);
            const auto duration = kt->first - it->first;
            valueSum += it->second * duration;
            timeSum += duration;
        }

        const auto duration = timestamp - readings.back().first;
        valueSum += readings.back().second * duration;
        timeSum += duration;

        return ReadingItem{timestamp, valueSum / timeSum};
    }
};

class Metric::FunctionSummation : public Metric::CollectionFunction
{
  public:
    ReadingItem calculate(const std::vector<ReadingItem>& readings,
                          uint64_t timestamp) const override
    {
        auto valueSum = 0.;
        for (auto it = readings.begin(); it != std::prev(readings.end()); ++it)
        {
            const auto kt = std::next(it);
            const auto duration = kt->first - it->first;
            valueSum += it->second * duration;
        }

        const auto duration = timestamp - readings.back().first;
        valueSum += readings.back().second * duration;

        return ReadingItem{timestamp, valueSum};
    }
};

class Metric::CollectionData
{
  public:
    virtual ~CollectionData() = default;

    virtual ReadingItem update(uint64_t timestamp) = 0;
    virtual ReadingItem update(uint64_t timestamp, double value) = 0;
};

class Metric::DataPoint : public Metric::CollectionData
{
  public:
    ReadingItem update(uint64_t timestamp) override
    {
        return ReadingItem{lastTimestamp, lastReading};
    }

    ReadingItem update(uint64_t timestamp, double reading) override
    {
        lastTimestamp = timestamp;
        lastReading = reading;
        return update(timestamp);
    }

  private:
    uint64_t lastTimestamp = 0u;
    double lastReading = 0.;
};

class Metric::DataInterval : public Metric::CollectionData
{
  public:
    DataInterval(std::unique_ptr<Metric::CollectionFunction> function,
                 CollectionDuration duration) :
        function(std::move(function)),
        duration(duration)
    {}

    ReadingItem update(uint64_t timestamp) override
    {
        if (readings.size() > 0)
        {
            auto it = readings.begin();
            for (auto kt = std::next(readings.rbegin()); kt != readings.rend();
                 ++kt)
            {
                const auto [nextItemTimestamp, nextItemReading] =
                    *std::prev(kt);
                if (timestamp >= nextItemTimestamp &&
                    static_cast<uint64_t>(timestamp - nextItemTimestamp) >
                        duration.count())
                {
                    it = kt.base();
                    break;
                }
            }
            readings.erase(readings.begin(), it);

            if (timestamp > duration.count())
            {
                readings.front().first = std::max(readings.front().first,
                                                  timestamp - duration.count());
            }
        }

        return function->calculate(readings, timestamp);
    }

    ReadingItem update(uint64_t timestamp, double reading) override
    {
        readings.emplace_back(timestamp, reading);
        return update(timestamp);
    }

  private:
    std::unique_ptr<Metric::CollectionFunction> function;
    std::vector<ReadingItem> readings;
    CollectionDuration duration;
};

class Metric::DataStartup : public Metric::CollectionData
{
  public:
    DataStartup(std::unique_ptr<Metric::CollectionFunction> function) :
        function(std::move(function))
    {}

    ReadingItem update(uint64_t timestamp) override
    {
        return function->calculate(readings, timestamp);
    }

    ReadingItem update(uint64_t timestamp, double reading) override
    {
        readings.emplace_back(timestamp, reading);
        return function->calculate(readings, timestamp);
    }

  private:
    std::unique_ptr<Metric::CollectionFunction> function;
    std::vector<ReadingItem> readings;
};

Metric::Metric(std::shared_ptr<interfaces::Sensor> sensor,
               OperationType operationType, std::string id,
               std::string metadata, CollectionTimeScope timeScope,
               CollectionDuration duration,
               std::unique_ptr<interfaces::Clock> clock) :
    sensor(std::move(sensor)),
    operationType(std::move(operationType)), id(std::move(id)),
    metadata(std::move(metadata)), collectionTimeScope(timeScope),
    collectionDuration(duration), collectionAlgorithm(makeCollectionData(
                                      makeCollectionFunction(operationType),
                                      collectionTimeScope, collectionDuration)),
    clock(std::move(clock))
{}

Metric::~Metric() = default;

void Metric::initialize()
{
    sensor->registerForUpdates(weak_from_this());
}

MetricValue Metric::getReading() const
{
    MetricValue mv{id, metadata, 0., 0u};
    std::tie(mv.timestamp, mv.value) =
        collectionAlgorithm->update(clock->timestamp());
    return mv;
}

void Metric::sensorUpdated(interfaces::Sensor& notifier, uint64_t timestamp)
{
    assertSensor(notifier);
    collectionAlgorithm->update(timestamp);
}

void Metric::sensorUpdated(interfaces::Sensor& notifier, uint64_t timestamp,
                           double value)
{
    assertSensor(notifier);
    collectionAlgorithm->update(timestamp, value);
}

void Metric::assertSensor(interfaces::Sensor& notifier) const
{
    if (sensor.get() != &notifier)
    {
        throw std::out_of_range("unknown sensor");
    }
}

LabeledMetricParameters Metric::dumpConfiguration() const
{
    auto sensorPath =
        LabeledSensorParameters(sensor->id().service, sensor->id().path);
    return LabeledMetricParameters(std::move(sensorPath), operationType, id,
                                   metadata, collectionTimeScope,
                                   collectionDuration);
}

std::unique_ptr<Metric::CollectionFunction>
    Metric::makeCollectionFunction(OperationType operationType)
{
    using namespace std::string_literals;

    switch (operationType)
    {
        case OperationType::single:
            return std::make_unique<FunctionSingle>();
        case OperationType::min:
            return std::make_unique<FunctionMinimum>();
        case OperationType::max:
            return std::make_unique<FunctionMaximum>();
        case OperationType::avg:
            return std::make_unique<FunctionAverage>();
        case OperationType::sum:
            return std::make_unique<FunctionSummation>();
        default:
            throw std::runtime_error("op: "s +
                                     utils::enumToString(operationType) +
                                     " is not supported"s);
    }
}

std::unique_ptr<Metric::CollectionData>
    Metric::makeCollectionData(std::unique_ptr<CollectionFunction> cf,
                               CollectionTimeScope timeScope,
                               CollectionDuration duration)
{
    using namespace std::string_literals;

    switch (timeScope)
    {
        case CollectionTimeScope::interval:
            return std::make_unique<DataInterval>(std::move(cf), duration);
        case CollectionTimeScope::point:
            return std::make_unique<DataPoint>();
        case CollectionTimeScope::startup:
            return std::make_unique<DataStartup>(std::move(cf));
        default:
            throw std::runtime_error("timeScope: "s +
                                     utils::enumToString(timeScope) +
                                     " is not supported"s);
    }
}
