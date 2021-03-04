#include "metric.hpp"

#include "types/types.hpp"
#include "utils/json.hpp"
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
    virtual ReadingItem
        calculateForStartupInterval(std::vector<ReadingItem>& readings,
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

    ReadingItem calculateForStartupInterval(std::vector<ReadingItem>& readings,
                                            uint64_t timestamp) const override
    {
        readings.assign({readings.back()});
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

    ReadingItem calculateForStartupInterval(std::vector<ReadingItem>& readings,
                                            uint64_t timestamp) const override
    {
        readings.assign({ReadingItem(calculate(readings, timestamp))});
        return readings.back();
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

    ReadingItem calculateForStartupInterval(std::vector<ReadingItem>& readings,
                                            uint64_t timestamp) const override
    {
        readings.assign({ReadingItem(calculate(readings, timestamp))});
        return readings.back();
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

    ReadingItem calculateForStartupInterval(std::vector<ReadingItem>& readings,
                                            uint64_t timestamp) const override
    {
        auto result = calculate(readings, timestamp);
        if (std::isfinite(result.second))
        {
            readings.assign({ReadingItem(readings.front().first, result.second),
                             ReadingItem(timestamp, readings.back().second)});
        }
        return result;
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

    ReadingItem calculateForStartupInterval(std::vector<ReadingItem>& readings,
                                            uint64_t timestamp) const override
    {
        auto result = calculate(readings, timestamp);
        if (std::isfinite(result.second) && timestamp > 0u)
        {
            readings.assign({ReadingItem(timestamp - 1u, result.second),
                             ReadingItem(timestamp, readings.back().second)});
        }
        return result;
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
    DataInterval(std::shared_ptr<Metric::CollectionFunction> function,
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
                        duration.t.count())
                {
                    it = kt.base();
                    break;
                }
            }
            readings.erase(readings.begin(), it);

            if (timestamp > duration.t.count())
            {
                readings.front().first = std::max(
                    readings.front().first, timestamp - duration.t.count());
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
    std::shared_ptr<Metric::CollectionFunction> function;
    std::vector<ReadingItem> readings;
    CollectionDuration duration;
};

class Metric::DataStartup : public Metric::CollectionData
{
  public:
    DataStartup(std::shared_ptr<Metric::CollectionFunction> function) :
        function(std::move(function))
    {}

    ReadingItem update(uint64_t timestamp) override
    {
        return function->calculateForStartupInterval(readings, timestamp);
    }

    ReadingItem update(uint64_t timestamp, double reading) override
    {
        readings.emplace_back(timestamp, reading);
        return function->calculateForStartupInterval(readings, timestamp);
    }

  private:
    std::shared_ptr<Metric::CollectionFunction> function;
    std::vector<ReadingItem> readings;
};

Metric::Metric(std::vector<std::shared_ptr<interfaces::Sensor>> sensorsIn,
               OperationType operationTypeIn, std::string idIn,
               std::string metadataIn, CollectionTimeScope timeScopeIn,
               CollectionDuration collectionDurationIn,
               std::unique_ptr<interfaces::Clock> clockIn) :
    id(idIn),
    metadata(metadataIn),
    readings(sensorsIn.size(),
             MetricValue{std::move(idIn), std::move(metadataIn), 0., 0u}),
    sensors(std::move(sensorsIn)), operationType(operationTypeIn),
    collectionTimeScope(timeScopeIn), collectionDuration(collectionDurationIn),
    collectionAlgorithms(makeCollectionData(sensors.size(), operationType,
                                            collectionTimeScope,
                                            collectionDuration)),
    clock(std::move(clockIn))
{
    try
    {
        const nlohmann::json parsedMetadata = nlohmann::json::parse(metadata);
        if (const auto metricProperties =
                utils::readJson<std::vector<std::string>>(parsedMetadata,
                                                          "MetricProperties"))
        {
            if (readings.size() == metricProperties->size())
            {
                for (size_t i = 0; i < readings.size(); ++i)
                {
                    readings[i].metadata = (*metricProperties)[i];
                }
            }
        }
    }
    catch (const nlohmann::json::parse_error& e)
    {}
}

Metric::~Metric() = default;

void Metric::initialize()
{
    for (const auto& sensor : sensors)
    {
        sensor->registerForUpdates(weak_from_this());
    }
}

std::vector<MetricValue> Metric::getReadings() const
{
    const auto timestamp = clock->timestamp();

    auto resultReadings = readings;

    for (size_t i = 0; i < resultReadings.size(); ++i)
    {
        std::tie(resultReadings[i].timestamp, resultReadings[i].value) =
            collectionAlgorithms[i]->update(timestamp);
    }

    return resultReadings;
}

void Metric::sensorUpdated(interfaces::Sensor& notifier, uint64_t timestamp)
{
    findAssociatedData(notifier).update(timestamp);
}

void Metric::sensorUpdated(interfaces::Sensor& notifier, uint64_t timestamp,
                           double value)
{
    findAssociatedData(notifier).update(timestamp, value);
}

Metric::CollectionData& Metric::findAssociatedData(interfaces::Sensor& notifier)
{
    auto it = std::find_if(
        sensors.begin(), sensors.end(),
        [&notifier](const auto& sensor) { return sensor.get() == &notifier; });
    auto index = std::distance(sensors.begin(), it);
    return *collectionAlgorithms.at(index);
}

LabeledMetricParameters Metric::dumpConfiguration() const
{
    auto sensorPath = utils::transform(sensors, [this](const auto& sensor) {
        return LabeledSensorParameters(sensor->id().service, sensor->id().path);
    });

    return LabeledMetricParameters(std::move(sensorPath), operationType, id,
                                   metadata, collectionTimeScope,
                                   collectionDuration);
}

std::shared_ptr<Metric::CollectionFunction>
    Metric::makeCollectionFunction(OperationType operationType)
{
    using namespace std::string_literals;

    switch (operationType)
    {
        case OperationType::single:
            return std::make_shared<FunctionSingle>();
        case OperationType::min:
            return std::make_shared<FunctionMinimum>();
        case OperationType::max:
            return std::make_shared<FunctionMaximum>();
        case OperationType::avg:
            return std::make_shared<FunctionAverage>();
        case OperationType::sum:
            return std::make_shared<FunctionSummation>();
        default:
            throw std::runtime_error("op: "s +
                                     utils::enumToString(operationType) +
                                     " is not supported"s);
    }
}

std::vector<std::unique_ptr<Metric::CollectionData>>
    Metric::makeCollectionData(size_t size, OperationType op,
                               CollectionTimeScope timeScope,
                               CollectionDuration duration)
{
    using namespace std::string_literals;

    std::vector<std::unique_ptr<Metric::CollectionData>> result;

    result.reserve(size);

    switch (timeScope)
    {
        case CollectionTimeScope::interval:
            std::generate_n(std::back_inserter(result), size,
                            [cf = makeCollectionFunction(op), duration] {
                                return std::make_unique<DataInterval>(cf,
                                                                      duration);
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
        default:
            throw std::runtime_error("timeScope: "s +
                                     utils::enumToString(timeScope) +
                                     " is not supported"s);
    }

    return result;
}
