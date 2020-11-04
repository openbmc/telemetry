#include "metric.hpp"

#include <algorithm>

Metric::Metric(std::vector<std::shared_ptr<interfaces::Sensor>> sensors,
               std::string operationType, std::string id,
               std::string metadata) :
    sensors(std::move(sensors)),
    operationType(std::move(operationType)), id(std::move(id)),
    metadata(std::move(metadata))
{}

void Metric::initialize()
{
    readings = std::vector<MetricValue>(sensors.size(),
                                        MetricValue{id, metadata, 0., 0u});

    for (auto& sensor : sensors)
    {
        sensor->registerForUpdates(weak_from_this());
    }
}

const std::vector<MetricValue>& Metric::getReadings() const
{
    return readings;
}

void Metric::sensorUpdated(interfaces::Sensor& sensor, uint64_t timestamp)
{
    MetricValue& mv = findMetric(sensor);
    mv.timestamp = timestamp;
}

void Metric::sensorUpdated(interfaces::Sensor& sensor, uint64_t timestamp,
                           double value)
{
    MetricValue& mv = findMetric(sensor);
    mv.timestamp = timestamp;
    mv.value = value;
}

MetricValue& Metric::findMetric(interfaces::Sensor& sensor)
{
    auto it =
        std::find_if(sensors.begin(), sensors.end(),
                     [&sensor](const auto& s) { return s.get() == &sensor; });
    auto index = std::distance(sensors.begin(), it);
    return readings.at(index);
}
