#include "metric.hpp"

#include "types/types.hpp"
#include "utils/labeled_tuple.hpp"
#include "utils/transform.hpp"

#include <algorithm>

Metric::Metric(std::vector<std::shared_ptr<interfaces::Sensor>> sensorsIn,
               OperationType operationTypeIn, std::string idIn,
               std::string metadataIn, CollectionTimeScope timeScopeIn,
               CollectionDuration collectionDurationIn) :
    id(idIn),
    metadata(metadataIn),
    readings(sensorsIn.size(),
             MetricValue{std::move(idIn), std::move(metadataIn), 0., 0u}),
    sensors(std::move(sensorsIn)), operationType(operationTypeIn),
    timeScope(timeScopeIn), collectionDuration(collectionDurationIn)
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
                readings[i].metadata = nlohmann::json(readingMetadata).dump();
            }
        }
    }
    catch (const nlohmann::json::parse_error&)
    {}
}

void Metric::initialize()
{
    for (const auto& sensor : sensors)
    {
        sensor->registerForUpdates(weak_from_this());
    }
}

const std::vector<MetricValue>& Metric::getReadings() const
{
    return readings;
}

void Metric::sensorUpdated(interfaces::Sensor& notifier, uint64_t timestamp)
{
    MetricValue& mv = findMetric(notifier);
    mv.timestamp = timestamp;
}

void Metric::sensorUpdated(interfaces::Sensor& notifier, uint64_t timestamp,
                           double value)
{
    MetricValue& mv = findMetric(notifier);
    mv.timestamp = timestamp;
    mv.value = value;
}

MetricValue& Metric::findMetric(interfaces::Sensor& notifier)
{
    auto it = std::find_if(
        sensors.begin(), sensors.end(),
        [&notifier](const auto& sensor) { return sensor.get() == &notifier; });
    auto index = std::distance(sensors.begin(), it);
    return readings.at(index);
}

LabeledMetricParameters Metric::dumpConfiguration() const
{
    auto sensorPath = utils::transform(sensors, [this](const auto& sensor) {
        return LabeledSensorParameters(sensor->id().service, sensor->id().path);
    });

    return LabeledMetricParameters(std::move(sensorPath), operationType, id,
                                   metadata, timeScope, collectionDuration);
}
