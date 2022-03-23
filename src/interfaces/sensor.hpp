#pragma once

#include "types/sensor_types.hpp"

#include <chrono>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

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

        bool operator<(const Id& other) const
        {
            return std::tie(type, service, path) <
                   std::tie(other.type, other.service, other.path);
        }

        inline std::string str() const
        {
            return type + ":" + service + ":" + path;
        }
    };

    virtual ~Sensor() = default;

    virtual Id id() const = 0;
    virtual std::string metadata() const = 0;
    virtual std::string getName() const = 0;
    virtual void registerForUpdates(
        const std::weak_ptr<SensorListener>&,
        SensorRegisterBehavior behavior = SensorRegisterBehavior::None) = 0;
    virtual void
        unregisterFromUpdates(const std::weak_ptr<SensorListener>&) = 0;

    virtual LabeledSensorInfo getLabeledSensorInfo() const = 0;
};

} // namespace interfaces

using Sensors = std::vector<std::shared_ptr<interfaces::Sensor>>;
