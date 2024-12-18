#pragma once

#include "types/collection_duration.hpp"
#include "types/collection_time_scope.hpp"
#include "types/duration_types.hpp"
#include "types/operation_type.hpp"

#include <memory>
#include <optional>
#include <vector>

namespace metrics
{

class CollectionData
{
  public:
    virtual ~CollectionData() = default;

    virtual std::optional<double> update(Milliseconds timestamp) = 0;
    virtual double update(Milliseconds timestamp, double value) = 0;
    bool updateLastValue(double value);

  private:
    std::optional<double> lastValue;
};

std::vector<std::unique_ptr<CollectionData>> makeCollectionData(
    size_t size, OperationType, CollectionTimeScope, CollectionDuration);

} // namespace metrics
