#pragma once

#include <string>
#include <vector>

namespace interfaces
{

class Trigger
{
  public:
    virtual ~Trigger() = default;

    virtual std::string getId() const = 0;
    virtual std::string getPath() const = 0;
    virtual const std::vector<std::string>& getReportIds() const = 0;
};

} // namespace interfaces
