#include "metric.hpp"

#include "types/types.hpp"
#include "utils/transform.hpp"

#include <algorithm>

Metric::Metric(std::shared_ptr<interfaces::Sensor> sensor,
               OperationType operationType, std::string id,
               std::string metadata, CollectionTimeScope, CollectionDuration) :
    sensor(std::move(sensor)),
    operationType(std::move(operationType)), reading{std::move(id),
                                                     std::move(metadata), 0.,
                                                     0u}
{}

void Metric::initialize()
{
    sensor->registerForUpdates(weak_from_this());
}

const MetricValue& Metric::getReading() const
{
    return reading;
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
    if (sensor.get() != &notifier)
    {
        throw std::out_of_range("unknown sensor");
    }
    return reading;
}

LabeledMetricParameters Metric::dumpConfiguration() const
{
    auto sensorPath =
        LabeledSensorParameters(sensor->id().service, sensor->id().path);
    return LabeledMetricParameters(std::move(sensorPath), operationType,
                                   reading.id, reading.metadata, timeScope,
                                   collectionDuration);
}
