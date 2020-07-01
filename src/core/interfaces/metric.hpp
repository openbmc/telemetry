#pragma once

#include "core/interfaces/sensor.hpp"
#include "core/operation_type.hpp"

#include <boost/beast/core/span.hpp>

#include <chrono>
#include <cmath>
#include <ctime>
#include <limits>
#include <string>

namespace core::interfaces
{

class Metric
{
  public:
    using ReadingsUpdated = std::function<void(void)>;

    struct Reading
    {
        double value;
        std::time_t timestamp;

        bool operator==(const Reading& other) const
        {
            return value == other.value && timestamp == other.timestamp;
        }
    };

    virtual ~Metric() = default;

    virtual OperationType type() const = 0;
    virtual std::string_view id() const = 0;
    virtual std::string_view metadata() const = 0;
    virtual const std::vector<std::shared_ptr<interfaces::Sensor>>&
        sensors() const = 0;
    virtual boost::beast::span<const std::optional<Reading>>
        readings() const = 0;
    virtual void async_read(ReadingsUpdated&& callback) = 0;
    virtual void registerForUpdates() = 0;
};

} // namespace core::interfaces
