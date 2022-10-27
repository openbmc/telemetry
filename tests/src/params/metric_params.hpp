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

    MetricParams& readings(std::vector<std::pair<Milliseconds, double>> value)
    {
        readingsProperty = std::move(value);
        return *this;
    }

    const std::vector<std::pair<Milliseconds, double>>& readings() const
    {
        return readingsProperty;
    }

    MetricParams& expectedReading(Milliseconds delta, double reading)
    {
        expectedReadingProperty = std::make_pair(delta, reading);
        return *this;
    }

    const std::pair<Milliseconds, double>& expectedReading() const
    {
        return expectedReadingProperty;
    }

    bool expectedIsTimerRequired() const
    {
        return expectedIsTimerRequiredProperty;
    }

    MetricParams& expectedIsTimerRequired(bool value)
    {
        expectedIsTimerRequiredProperty = value;
        return *this;
    }

  private:
    OperationType operationTypeProperty = {};
    CollectionTimeScope collectionTimeScopeProperty = {};
    CollectionDuration collectionDurationProperty =
        CollectionDuration(Milliseconds(0u));
    std::vector<std::pair<Milliseconds, double>> readingsProperty = {};
    std::pair<Milliseconds, double> expectedReadingProperty = {};
    bool expectedIsTimerRequiredProperty = true;
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
    os << " }, expectedReading: " << reading << "(" << timestamp.count()
       << "ms), expectedIsTimerRequired: " << std::boolalpha
       << mp.expectedIsTimerRequired() << " }";
    return os;
}
