#pragma once

namespace interfaces
{

class Threshold
{
  public:
    virtual ~Threshold() = default;

    virtual void initialize() = 0;
};

} // namespace interfaces
