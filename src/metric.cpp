#include "metric.hpp"

#include "details/collection_function.hpp"
#include "types/report_types.hpp"
#include "utils/labeled_tuple.hpp"
#include "utils/transform.hpp"

#include <algorithm>

class Metric::CollectionData
{
  public:
    using ReadingItem = details::ReadingItem;

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
    double lastReading = 0.0;
};

class Metric::DataInterval : public Metric::CollectionData
{
  public:
    DataInterval(std::shared_ptr<details::CollectionFunction> function,
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
                const auto& [nextItemTimestamp, nextItemReading] =
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
    std::shared_ptr<details::CollectionFunction> function;
    std::vector<ReadingItem> readings;
    CollectionDuration duration;
};

class Metric::DataStartup : public Metric::CollectionData
{
  public:
    DataStartup(std::shared_ptr<details::CollectionFunction> function) :
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
    std::shared_ptr<details::CollectionFunction> function;
    std::vector<ReadingItem> readings;
};

Metric::Metric(Sensors sensorsIn, OperationType operationTypeIn,
               std::string idIn, std::string metadataIn,
               CollectionTimeScope timeScopeIn,
               CollectionDuration collectionDurationIn,
               std::unique_ptr<interfaces::Clock> clockIn) :
    id(idIn),
    metadata(metadataIn),
    readings(sensorsIn.size(),
             MetricValue{std::move(idIn), std::move(metadataIn), 0.0, 0u}),
    sensors(std::move(sensorsIn)), operationType(operationTypeIn),
    collectionTimeScope(timeScopeIn), collectionDuration(collectionDurationIn),
    collectionAlgorithms(makeCollectionData(sensors.size(), operationType,
                                            collectionTimeScope,
                                            collectionDuration)),
    clock(std::move(clockIn))
{
    attemptUnpackJsonMetadata();
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
        return LabeledSensorParameters(sensor->id().service, sensor->id().path);
    });

    return LabeledMetricParameters(std::move(sensorPath), operationType, id,
                                   metadata, collectionTimeScope,
                                   collectionDuration);
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

void Metric::attemptUnpackJsonMetadata()
{
    using MetricMetadata =
        utils::LabeledTuple<std::tuple<std::vector<std::string>>,
                            utils::tstring::MetricProperties>;

    using ReadingMetadata =
        utils::LabeledTuple<std::tuple<std::string, std::string>,
                            utils::tstring::SensorDbusPath,
                            utils::tstring::SensorRedfishUri>;
    try
    {
        const MetricMetadata parsedMetadata =
            nlohmann::json::parse(metadata).get<MetricMetadata>();

        if (readings.size() == parsedMetadata.at_index<0>().size())
        {
            for (size_t i = 0; i < readings.size(); ++i)
            {
                ReadingMetadata readingMetadata{
                    sensors[i]->id().path, parsedMetadata.at_index<0>()[i]};
                readings[i].metadata = readingMetadata.dump();
            }
        }
    }
    catch (const nlohmann::json::exception&)
    {}
}
