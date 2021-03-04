#pragma once

#include "types/operation_type.hpp"

#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace details
{

using ReadingItem = std::pair<uint64_t, double>;

class CollectionFunction
{
  public:
    virtual ~CollectionFunction() = default;

    virtual ReadingItem calculate(const std::vector<ReadingItem>& readings,
                                  uint64_t timestamp) const = 0;
    virtual ReadingItem
        calculateForStartupInterval(std::vector<ReadingItem>& readings,
                                    uint64_t timestamp) const = 0;
};

std::shared_ptr<CollectionFunction> makeCollectionFunction(OperationType);

} // namespace details
