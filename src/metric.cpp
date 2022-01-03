#include "metric.hpp"

#include "details/collection_function.hpp"
#include "types/report_types.hpp"
#include "utils/labeled_tuple.hpp"
#include "utils/transform.hpp"

#include <sdbusplus/exception.hpp>

#include <algorithm>

class Metric::CollectionData
{
  public:
    using ReadingItem = details::ReadingItem;

    virtual ~CollectionData() = default;

    virtual std::optional<double> update(Milliseconds timestamp) = 0;
    virtual double update(Milliseconds timestamp, double value) = 0;
};

class Metric::DataPoint : public Metric::CollectionData
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

class Metric::DataInterval : public Metric::CollectionData
{
  public:
    DataInterval(std::shared_ptr<details::CollectionFunction> function,
                 CollectionDuration duration) :
        function(std::move(function)),
        duration(duration)
    {
        if (duration.t.count() == 0)
        {
            throw sdbusplus::exception::SdBusError(
                static_cast<int>(std::errc::invalid_argument),
                "Invalid CollectionDuration");
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

    std::shared_ptr<details::CollectionFunction> function;
    std::vector<ReadingItem> readings;
    CollectionDuration duration;
};

class Metric::DataStartup : public Metric::CollectionData
{
  public:
    explicit DataStartup(
        std::shared_ptr<details::CollectionFunction> function) :
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
    std::shared_ptr<details::CollectionFunction> function;
    std::vector<ReadingItem> readings;
};

Metric::Metric(Sensors sensorsIn, OperationType operationTypeIn,
               std::string idIn, CollectionTimeScope timeScopeIn,
               CollectionDuration collectionDurationIn,
               std::unique_ptr<interfaces::Clock> clockIn) :
    id(std::move(idIn)),
    sensors(std::move(sensorsIn)), operationType(operationTypeIn),
    collectionTimeScope(timeScopeIn), collectionDuration(collectionDurationIn),
    collectionAlgorithms(makeCollectionData(sensors.size(), operationType,
                                            collectionTimeScope,
                                            collectionDuration)),
    clock(std::move(clockIn))
{
    readings = utils::transform(sensors, [this](const auto& sensor) {
        return MetricValue{id, sensor->metadata(), 0.0, 0u};
    });
}

Metric::~Metric() = default;

void Metric::initialize()
{
    for (const auto& sensor : sensors)
    {
        sensor->registerForUpdates(weak_from_this());
    }
}

void Metric::deinitialize()
{
    for (const auto& sensor : sensors)
    {
        sensor->unregisterFromUpdates(weak_from_this());
    }
}

std::vector<MetricValue> Metric::getReadings() const
{
    const auto steadyTimestamp = clock->steadyTimestamp();
    const auto systemTimestamp = clock->systemTimestamp();

    auto resultReadings = readings;

    for (size_t i = 0; i < resultReadings.size(); ++i)
    {
        if (const auto value = collectionAlgorithms[i]->update(steadyTimestamp))
        {
            resultReadings[i].timestamp =
                std::chrono::duration_cast<Milliseconds>(systemTimestamp)
                    .count();
            resultReadings[i].value = *value;
        }
    }

    return resultReadings;
}

void Metric::sensorUpdated(interfaces::Sensor& notifier, Milliseconds timestamp)
{
    findAssociatedData(notifier).update(timestamp);
}

void Metric::sensorUpdated(interfaces::Sensor& notifier, Milliseconds timestamp,
                           double value)
{
    findAssociatedData(notifier).update(timestamp, value);
}

Metric::CollectionData&
    Metric::findAssociatedData(const interfaces::Sensor& notifier)
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
        return LabeledSensorParameters(sensor->id().service, sensor->id().path,
                                       sensor->metadata());
    });

    return LabeledMetricParameters(std::move(sensorPath), operationType, id,
                                   collectionTimeScope, collectionDuration);
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
            std::generate_n(
                std::back_inserter(result), size,
                [cf = details::makeCollectionFunction(op), duration] {
                    return std::make_unique<DataInterval>(cf, duration);
                });
            break;
        case CollectionTimeScope::point:
            std::generate_n(std::back_inserter(result), size,
                            [] { return std::make_unique<DataPoint>(); });
            break;
        case CollectionTimeScope::startup:
            std::generate_n(std::back_inserter(result), size,
                            [cf = details::makeCollectionFunction(op)] {
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

uint64_t Metric::sensorCount() const
{
    return sensors.size();
}
