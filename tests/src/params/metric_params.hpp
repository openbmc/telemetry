#pragma once

#include "types/collection_duration.hpp"
#include "types/collection_time_scope.hpp"
#include "types/operation_type.hpp"

#include <string>

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

  private:
    OperationType operationTypeProperty = {};
    std::string idProperty = "MetricId";
    std::string metadataProperty = "MetricMetadata";
    CollectionTimeScope collectionTimeScopeProperty = {};
    CollectionDuration collectionDurationProperty =
        CollectionDuration(Milliseconds(0u));
};
