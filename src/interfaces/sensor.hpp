#pragma once

#include <ostream>
#include <string>
#include <string_view>

namespace interfaces
{

class Sensor
{
  public:
    struct Id
    {
        Id(std::string_view type, std::string_view service,
           std::string_view path) :
            type(type),
            service(service), path(path)
        {}

        std::string type;
        std::string service;
        std::string path;

        friend std::ostream& operator<<(std::ostream& os, const Id& id)
        {
            return os << id.type << ":" << id.service << ":" << id.path;
        }
    };

    virtual ~Sensor() = default;

    virtual Id id() const = 0;
};

} // namespace interfaces
