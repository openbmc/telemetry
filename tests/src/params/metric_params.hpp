#pragma once

#include "types/collection_duration.hpp"
#include "types/collection_time_scope.hpp"
#include "types/operation_type.hpp"

#include <chrono>
#include <cstdint>
#include <ostream>
#include <vector>

class MetricParams final
{
  public:
    MetricParams& operationType(OperationType op)
    {
        operationTypeProperty = op;
        return *this;
    }

    MetricParams& collectionTimeScope(CollectionTimeScope timeScope)
    {
        collectionTimeScopeProperty = timeScope;
        return *this;
    }

    OperationType operationType() const
    {
        return operationTypeProperty;
    }

    CollectionTimeScope collectionTimeScope() const
    {
        return collectionTimeScopeProperty;
    }

    MetricParams& collectionDuration(CollectionDuration duration)
    {
        collectionDurationProperty = duration;
        return *this;
    }

    CollectionDuration collectionDuration() const
    {
        return collectionDurationProperty;
    }

    MetricParams& readings(
        std::vector<std::pair<std::chrono::milliseconds, double>> value)
    {
        readingsProperty = std::move(value);
        return *this;
    }

    MetricParams& reading(std::chrono::milliseconds delta, double reading)
    {
        readingsProperty.emplace_back(delta, reading);
        return *this;
    }

    const std::vector<std::pair<std::chrono::milliseconds, double>>&
        readings() const
    {
        return readingsProperty;
    }

    MetricParams& expectedReading(std::chrono::milliseconds delta,
                                  double reading)
    {
        expectedReadingProperty = std::make_pair(delta, reading);
        return *this;
    }

    const std::pair<std::chrono::milliseconds, double>& expectedReading() const
    {
        return expectedReadingProperty;
    }

  private:
    OperationType operationTypeProperty = OperationType::single;
    CollectionTimeScope collectionTimeScopeProperty =
        CollectionTimeScope::point;
    CollectionDuration collectionDurationProperty{std::chrono::milliseconds(0)};
    std::vector<std::pair<std::chrono::milliseconds, double>> readingsProperty =
        {};
    std::pair<std::chrono::milliseconds, double> expectedReadingProperty = {};
};

inline std::ostream& operator<<(std::ostream& os, const MetricParams& mp)
{
    using utils::enumToString;

    os << "{ op: " << enumToString(mp.operationType())
       << ", timeScope: " << enumToString(mp.collectionTimeScope())
       << ", duration: " << mp.collectionDuration().count() << ", readings: { ";
    for (auto [timestamp, reading] : mp.readings())
    {
        os << reading << "(" << timestamp.count() << "ms), ";
    }

    auto [timestamp, reading] = mp.expectedReading();
    os << " }, expected: " << reading << "(" << timestamp.count() << "ms) }";
    return os;
}
