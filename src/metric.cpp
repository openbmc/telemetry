#include "metric.hpp"

#include "metrics/collection_data.hpp"
#include "types/report_types.hpp"
#include "utils/labeled_tuple.hpp"
#include "utils/transform.hpp"

#include <sdbusplus/exception.hpp>

#include <algorithm>

Metric::Metric(Sensors sensorsIn, OperationType operationTypeIn,
               std::string idIn, CollectionTimeScope timeScopeIn,
               CollectionDuration collectionDurationIn,
               std::unique_ptr<interfaces::Clock> clockIn) :
    id(std::move(idIn)),
    sensors(std::move(sensorsIn)), operationType(operationTypeIn),
    collectionTimeScope(timeScopeIn), collectionDuration(collectionDurationIn),
    collectionAlgorithms(
        metrics::makeCollectionData(sensors.size(), operationType,
                                    collectionTimeScope, collectionDuration)),
    clock(std::move(clockIn))
{
    readings = utils::transform(sensors, [this](const auto& sensor) {
        return MetricValue{id, sensor->metadata(), 0.0, 0u};
    });
}

void Metric::registerForUpdates(interfaces::MetricListener& listener)
{
    listeners.emplace_back(listener);
}

void Metric::unregisterFromUpdates(interfaces::MetricListener& listener)
{
    listeners.erase(
        std::remove_if(listeners.begin(), listeners.end(),
                       [&listener](const interfaces::MetricListener& item) {
                           return &item == &listener;
                       }),
        listeners.end());
}

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

void Metric::sensorUpdated(interfaces::Sensor& notifier, Milliseconds timestamp,
                           double value)
{
    auto& data = findAssociatedData(notifier);
    double newValue = data.update(timestamp, value);

    if (data.updateLastValue(newValue))
    {
        for (interfaces::MetricListener& listener : listeners)
        {
            listener.metricUpdated();
        }
    }
}

metrics::CollectionData&
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

uint64_t Metric::sensorCount() const
{
    return sensors.size();
}

void Metric::updateReadings(Milliseconds timestamp)
{
    for (auto& data : collectionAlgorithms)
    {
        if (std::optional<double> newValue = data->update(timestamp))
        {
            if (data->updateLastValue(*newValue))
            {
                for (interfaces::MetricListener& listener : listeners)
                {
                    listener.metricUpdated();
                }
                return;
            }
        }
    }
}

bool Metric::isTimerRequired() const
{
    if (collectionTimeScope == CollectionTimeScope::point)
    {
        return false;
    }

    if (collectionTimeScope == CollectionTimeScope::startup &&
        (operationType == OperationType::min ||
         operationType == OperationType::max))
    {
        return false;
    }

    return true;
}
