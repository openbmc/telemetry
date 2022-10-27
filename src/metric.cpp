#include "metric.hpp"

#include "metrics/collection_data.hpp"
#include "types/report_types.hpp"
#include "types/sensor_types.hpp"
#include "utils/labeled_tuple.hpp"
#include "utils/transform.hpp"

#include <sdbusplus/exception.hpp>

#include <algorithm>

Metric::Metric(Sensors sensorsIn, OperationType operationTypeIn,
               CollectionTimeScope timeScopeIn,
               CollectionDuration collectionDurationIn,
               std::unique_ptr<interfaces::Clock> clockIn) :
    sensors(std::move(sensorsIn)),
    operationType(operationTypeIn), collectionTimeScope(timeScopeIn),
    collectionDuration(collectionDurationIn),
    collectionAlgorithms(
        metrics::makeCollectionData(sensors.size(), operationType,
                                    collectionTimeScope, collectionDuration)),
    clock(std::move(clockIn))
{}

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

const std::vector<MetricValue>& Metric::getUpdatedReadings()
{
    const auto steadyTimestamp = clock->steadyTimestamp();
    const auto systemTimestamp =
        std::chrono::duration_cast<Milliseconds>(clock->systemTimestamp())
            .count();

    for (size_t i = 0; i < collectionAlgorithms.size(); ++i)
    {
        if (const auto value = collectionAlgorithms[i]->update(steadyTimestamp))
        {
            if (i < readings.size())
            {
                readings[i].timestamp = systemTimestamp;
                readings[i].value = *value;
            }
            else
            {
                if (i > readings.size())
                {
                    const auto idx = readings.size();
                    std::swap(collectionAlgorithms[i],
                              collectionAlgorithms[idx]);
                    std::swap(sensors[i], sensors[idx]);
                    i = idx;
                }

                readings.emplace_back(sensors[i]->metadata(), *value,
                                      systemTimestamp);
            }
        }
    }

    return readings;
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
        return LabeledSensorInfo(sensor->id().service, sensor->id().path,
                                 sensor->metadata());
    });

    return LabeledMetricParameters(std::move(sensorPath), operationType,
                                   collectionTimeScope, collectionDuration);
}

uint64_t Metric::metricCount() const
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
