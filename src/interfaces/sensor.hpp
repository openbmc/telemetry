#pragma once

#include <ostream>
#include <string>
#include <string_view>
#include <tuple>

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

        inline std::string str() const
        {
            return type + ":" + service + ":" + path;
        }

        bool operator<(const Id& other) const
        {
            return std::tie(type, service, path) <
                   std::tie(other.type, other.service, other.path);
        }
    };

    virtual ~Sensor() = default;

    virtual Id id() const = 0;
};

} // namespace interfaces
