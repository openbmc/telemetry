#pragma once

#include "interfaces/metric.hpp"
#include "interfaces/sensor.hpp"
#include "interfaces/sensor_listener.hpp"

class Metric :
    public interfaces::Metric,
    public interfaces::SensorListener,
    public std::enable_shared_from_this<Metric>
{
  public:
    Metric(Sensors sensors, OperationType operationType, std::string id,
           std::string metadata, CollectionTimeScope, CollectionDuration);

    void initialize() override;
    const std::vector<MetricValue>& getReadings() const override;
    void sensorUpdated(interfaces::Sensor&, uint64_t) override;
    void sensorUpdated(interfaces::Sensor&, uint64_t, double value) override;
    LabeledMetricParameters dumpConfiguration() const override;

  private:
    void tryUnpackJsonMetadata();

    MetricValue& findMetric(interfaces::Sensor&);

    std::string id;
    std::string metadata;
    std::vector<MetricValue> readings;
    Sensors sensors;
    OperationType operationType;
    CollectionTimeScope timeScope;
    CollectionDuration collectionDuration;
};
