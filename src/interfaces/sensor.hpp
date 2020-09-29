#pragma once

#include <chrono>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>

namespace interfaces
{

class SensorListener;

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
    };

    virtual ~Sensor() = default;

    virtual Id id() const = 0;
    virtual void async_read() = 0;
    virtual void registerForUpdates(const std::weak_ptr<SensorListener>&) = 0;
    virtual void schedule(std::chrono::milliseconds) = 0;
    virtual void stop() = 0;
};

} // namespace interfaces
