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
    Metric(std::shared_ptr<interfaces::Sensor> sensor,
           OperationType operationType, std::string id, std::string metadata,
           CollectionTimeScope, CollectionDuration,
           std::unique_ptr<interfaces::Clock>);
    ~Metric();

    void initialize() override;
    MetricValue getReading() const override;
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

    static std::unique_ptr<CollectionFunction>
        makeCollectionFunction(OperationType);
    static std::unique_ptr<CollectionData>
        makeCollectionData(std::unique_ptr<CollectionFunction>,
                           CollectionTimeScope, CollectionDuration);

    void assertSensor(interfaces::Sensor&) const;

    std::shared_ptr<interfaces::Sensor> sensor;
    OperationType operationType;
    std::string id;
    std::string metadata;
    CollectionTimeScope collectionTimeScope;
    CollectionDuration collectionDuration;
    std::unique_ptr<CollectionData> collectionAlgorithm;
    std::unique_ptr<interfaces::Clock> clock;
};
