#pragma once

#include "types/duration_types.hpp"

#include <cstdint>
#include <string>

namespace interfaces
{

class TriggerAction
{
  public:
    virtual ~TriggerAction() = default;

    virtual void commit(const std::string& id, Milliseconds timestamp,
                        double value) = 0;
};
} // namespace interfaces
