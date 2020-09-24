#pragma once

#include <string>
#include <string_view>

namespace interfaces
{

class Sensor
{
  public:
    struct Id
    {
        Id(std::string_view type, std::string_view name) :
            type(type), name(name)
        {}

        std::string type;
        std::string name;
    };

    virtual ~Sensor() = default;

    virtual Id id() const = 0;
};

} // namespace interfaces
