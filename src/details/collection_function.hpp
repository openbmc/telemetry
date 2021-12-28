#pragma once

#include "types/duration_types.hpp"
#include "types/operation_type.hpp"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace details
{

using ReadingItem = std::pair<Milliseconds, double>;

class CollectionFunction
{
  public:
    virtual ~CollectionFunction() = default;

    virtual double calculate(const std::vector<ReadingItem>& readings,
                             Milliseconds timestamp) const = 0;
    virtual double
        calculateForStartupInterval(std::vector<ReadingItem>& readings,
                                    Milliseconds timestamp) const = 0;
};

std::shared_ptr<CollectionFunction> makeCollectionFunction(OperationType);

} // namespace details
