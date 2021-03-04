#pragma once

#include "interfaces/clock.hpp"
#include "interfaces/metric.hpp"
#include "interfaces/sensor.hpp"
#include "interfaces/sensor_listener.hpp"

class Metric :
    public interfaces::Metric,
    public interfaces::SensorListener,
    public std::enable_shared_from_this<Metric>
{
  public:
    Metric(std::vector<std::shared_ptr<interfaces::Sensor>> sensors,
           OperationType operationType, std::string id, std::string metadata,
           CollectionTimeScope, CollectionDuration,
           std::unique_ptr<interfaces::Clock>);
    ~Metric();

    void initialize() override;
    std::vector<MetricValue> getReadings() const override;
    void sensorUpdated(interfaces::Sensor&, uint64_t) override;
    void sensorUpdated(interfaces::Sensor&, uint64_t, double value) override;
    LabeledMetricParameters dumpConfiguration() const override;

  private:
    class CollectionData;
    class DataPoint;
    class DataInterval;
    class DataStartup;
    class CollectionFunction;
    class FunctionSingle;
    class FunctionMinimum;
    class FunctionMaximum;
    class FunctionAverage;
    class FunctionSummation;

    static std::shared_ptr<CollectionFunction>
        makeCollectionFunction(OperationType);
    static std::vector<std::unique_ptr<CollectionData>>
        makeCollectionData(size_t size, OperationType, CollectionTimeScope,
                           CollectionDuration);

    CollectionData& findAssociatedData(interfaces::Sensor& notifier);

    std::string id;
    std::string metadata;
    std::vector<MetricValue> readings;
    std::vector<std::shared_ptr<interfaces::Sensor>> sensors;
    OperationType operationType;
    CollectionTimeScope collectionTimeScope;
    CollectionDuration collectionDuration;
    std::vector<std::unique_ptr<CollectionData>> collectionAlgorithms;
    std::unique_ptr<interfaces::Clock> clock;
};
