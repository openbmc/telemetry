#pragma once

#include <string>

namespace interfaces
{

class Report
{
  public:
    virtual ~Report() = default;

    virtual std::string getName() const = 0;
    virtual std::string getPath() const = 0;
    virtual void updateReadings() = 0;
};
} // namespace interfaces
