#pragma once

#include "types/report_types.hpp"

#include <string>

namespace interfaces
{

class Report
{
  public:
    virtual ~Report() = default;

    virtual std::string getId() const = 0;
    virtual std::string getPath() const = 0;
    virtual void updateReadings() = 0;
    virtual void updateTriggerIds(const std::string& triggerId,
                                  TriggerIdUpdate updateType) = 0;
};
} // namespace interfaces
