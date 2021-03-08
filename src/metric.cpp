#include "metric.hpp"

#include "types/types.hpp"
#include "utils/transform.hpp"

#include <algorithm>

Metric::Metric(std::vector<std::shared_ptr<interfaces::Sensor>> sensors,
               OperationType operationType, std::string id,
               std::string metadata, CollectionTimeScope timeScope,
               CollectionDuration collectionDuration) :
    readings(sensors.size(),
             MetricValue{std::move(id), std::move(metadata), 0., 0u}),
    sensors(std::move(sensors)), operationType(operationType),
    timeScope(timeScope), collectionDuration(collectionDuration)
{}

void Metric::initialize()
{
    for (const auto& sensor : sensors)
    {
        sensor->registerForUpdates(weak_from_this());
    }
}

const MetricValue& Metric::getReading() const
{
    return readings.front();
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

    return LabeledMetricParameters(
        std::move(sensorPath), operationType, readings.front().id,
        readings.front().metadata, timeScope, collectionDuration);
}
