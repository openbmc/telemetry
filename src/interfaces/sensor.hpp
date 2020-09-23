#pragma once

#include <string>

namespace interfaces
{

class Sensor
{
  public:
    struct Id
    {
        std::string type;
        std::string name;
    };

    virtual ~Sensor() = default;

    virtual Id id() const = 0;
};

} // namespace interfaces
