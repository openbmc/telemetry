#pragma once

#include "interfaces/clock.hpp"
#include "interfaces/metric.hpp"
#include "interfaces/metric_listener.hpp"
#include "interfaces/sensor.hpp"
#include "interfaces/sensor_listener.hpp"
#include "metrics/collection_data.hpp"
#include "types/collection_duration.hpp"

class Metric :
    public interfaces::Metric,
    public interfaces::SensorListener,
    public std::enable_shared_from_this<Metric>
{
  public:
    Metric(Sensors sensors, OperationType operationType, CollectionTimeScope,
           CollectionDuration, std::unique_ptr<interfaces::Clock>);

    void initialize() override;
    void deinitialize() override;
    const std::vector<MetricValue>& getUpdatedReadings() override;
    void sensorUpdated(interfaces::Sensor&, Milliseconds,
                       double value) override;
    LabeledMetricParameters dumpConfiguration() const override;
    uint64_t metricCount() const override;
    void registerForUpdates(interfaces::MetricListener& listener) override;
    void unregisterFromUpdates(interfaces::MetricListener& listener) override;
    void updateReadings(Milliseconds) override;
    bool isTimerRequired() const override;

  private:
    metrics::CollectionData&
        findAssociatedData(const interfaces::Sensor& notifier);

    std::vector<MetricValue> readings;
    Sensors sensors;
    OperationType operationType;
    CollectionTimeScope collectionTimeScope;
    CollectionDuration collectionDuration;
    std::vector<std::unique_ptr<metrics::CollectionData>> collectionAlgorithms;
    std::unique_ptr<interfaces::Clock> clock;
    std::vector<std::reference_wrapper<interfaces::MetricListener>> listeners;
};
