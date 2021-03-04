#pragma once

#include "types/collection_duration.hpp"
#include "types/collection_time_scope.hpp"
#include "types/operation_type.hpp"

#include <chrono>
#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

class MetricParams final
{
  public:
    MetricParams& operationType(OperationType val)
    {
        operationTypeProperty = val;
        return *this;
    }

    const OperationType& operationType() const
    {
        return operationTypeProperty;
    }

    MetricParams& id(std::string val)
    {
        idProperty = std::move(val);
        return *this;
    }

    const std::string& id() const
    {
        return idProperty;
    }

    MetricParams& metadata(std::string val)
    {
        metadataProperty = std::move(val);
        return *this;
    }

    const std::string& metadata() const
    {
        return metadataProperty;
    }

    MetricParams& collectionTimeScope(CollectionTimeScope val)
    {
        collectionTimeScopeProperty = val;
        return *this;
    }

    const CollectionTimeScope& collectionTimeScope() const
    {
        return collectionTimeScopeProperty;
    }

    MetricParams& collectionDuration(CollectionDuration val)
    {
        collectionDurationProperty = val;
        return *this;
    }

    const CollectionDuration& collectionDuration() const
    {
        return collectionDurationProperty;
    }

    MetricParams& readings(
        std::vector<std::pair<DurationType, double>> value)
    {
        readingsProperty = std::move(value);
        return *this;
    }

    MetricParams& reading(DurationType delta, double reading)
    {
        readingsProperty.emplace_back(delta, reading);
        return *this;
    }

    const std::vector<std::pair<DurationType, double>>&
        readings() const
    {
        return readingsProperty;
    }

    MetricParams& expectedReading(DurationType delta,
                                  double reading)
    {
        expectedReadingProperty = std::make_pair(delta, reading);
        return *this;
    }

    const std::pair<DurationType, double>& expectedReading() const
    {
        return expectedReadingProperty;
    }

  private:
    OperationType operationTypeProperty = {};
    std::string idProperty = "MetricId";
    std::string metadataProperty = "MetricMetadata";
    CollectionTimeScope collectionTimeScopeProperty = {};
    CollectionDuration collectionDurationProperty =
        CollectionDuration(DurationType(0u));
    std::vector<std::pair<DurationType, double>> readingsProperty =
        {};
    std::pair<DurationType, double> expectedReadingProperty = {};
};

inline std::ostream& operator<<(std::ostream& os, const MetricParams& mp)
{
    using utils::enumToString;

    os << "{ op: " << enumToString(mp.operationType())
       << ", timeScope: " << enumToString(mp.collectionTimeScope())
       << ", duration: " << mp.collectionDuration().t.count()
       << ", readings: { ";
    for (auto [timestamp, reading] : mp.readings())
    {
        os << reading << "(" << timestamp.count() << "ms), ";
    }

    auto [timestamp, reading] = mp.expectedReading();
    os << " }, expected: " << reading << "(" << timestamp.count() << "ms) }";
    return os;
}
