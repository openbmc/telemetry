#include "metric.hpp"

#include "types/report_types.hpp"
#include "utils/json.hpp"
#include "utils/transform.hpp"

#include <algorithm>

Metric::Metric(Sensors sensorsIn, OperationType operationTypeIn,
               std::string idIn, std::string metadataIn,
               CollectionTimeScope timeScopeIn,
               CollectionDuration collectionDurationIn) :
    id(idIn),
    metadata(metadataIn),
    readings(sensorsIn.size(),
             MetricValue{std::move(idIn), std::move(metadataIn), 0., 0u}),
    sensors(std::move(sensorsIn)), operationType(operationTypeIn),
    timeScope(timeScopeIn), collectionDuration(collectionDurationIn)
{
    tryUnpackJsonMetadata();
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

void Metric::tryUnpackJsonMetadata()
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
