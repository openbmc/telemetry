#pragma once

#include <string>

namespace interfaces
{

class Trigger
{
  public:
    virtual ~Trigger() = default;

    virtual std::string getName() const = 0;
    virtual std::string getPath() const = 0;
};

} // namespace interfaces
