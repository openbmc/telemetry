#pragma once

#include <cstdint>
#include <string>

namespace interfaces
{

class TriggerAction
{
  public:
    virtual ~TriggerAction() = default;

    virtual void commit(const std::string& id, uint64_t timestamp,
                        double value) = 0;
};
} // namespace interfaces
