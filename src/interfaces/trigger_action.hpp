#pragma once

#include <cstdint>

namespace interfaces
{

class TriggerAction
{
  public:
    virtual ~TriggerAction() = default;

    virtual void commit(uint64_t timestamp, double value) = 0;
};
} // namespace interfaces
