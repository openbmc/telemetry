#pragma once

#include "interfaces/metric.hpp"
#include "utils/compare_underlying_objects.hpp"

#include <exception>
#include <memory>
#include <string>
#include <vector>

namespace core
{

class Metric :
    public interfaces::Metric,
    public std::enable_shared_from_this<Metric>
{
  public:
    Metric(const std::string& metricId, const std::string& metadata,
           const std::vector<std::shared_ptr<interfaces::Sensor>>&& sensors,
           const OperationType type) :
        metricId_(metricId),
        metadata_(metadata), sensors_(std::move(sensors)), type_(type)
    {
        if (OperationType::single != type || sensors.size() != 1)
        {
            // TODO: Specialize exceptions
            throw std::runtime_error(
                "Currently only single-sensor metric is supported");
        }
    }

    bool operator==(const interfaces::Metric& other) const override
    {
        return type() == other.type() && id() == other.id() &&
               metadata() == other.metadata() &&
               std::equal(sensors().begin(), sensors().end(),
                          other.sensors().begin(), other.sensors().end(),
                          utils::CompareUnderlyingObjects()) &&
               std::equal(readings().begin(), readings().end(),
                          other.readings().begin(), other.readings().end(),
                          utils::CompareUnderlyingObjects());
    }

    OperationType type() const override
    {
        return type_;
    }

    std::string_view id() const override
    {
        return metricId_;
    }

    std::string_view metadata() const override
    {
        return metadata_;
    }

    const std::vector<std::shared_ptr<interfaces::Sensor>>&
        sensors() const override
    {
        return sensors_;
    }

    boost::beast::span<const std::optional<Reading>> readings() const override
    {
        return {readings_.data(), readings_.size()};
    }

    void async_read(ReadingsUpdatedCallback&& callback) override
    {
        // Right now only passthrough, later - operation after all complete
        auto sharedCallback =
            std::make_shared<std::decay_t<ReadingsUpdatedCallback>>(
                std::move(callback));

        // TODO: Implement for multiple sensors (perform operation)
        sensors_[0]->async_read(
            [me = shared_from_this(), sharedCallback](
                const boost::system::error_code& e, const double value) {
                if (!e)
                {
                    me->readings_[0] = Reading{value, std::time(0)};
                }

                // TODO: if(thisIsLastReading)
                (*sharedCallback)();
            });
    }

    void registerForUpdates() override
    {
        sensors_[0]->registerForUpdates(
            [weakSelf = weak_from_this()](const boost::system::error_code& e,
                                          const double value) {
                if (!e)
                {
                    if (auto self = weakSelf.lock())
                    {
                        self->readings_[0] = Reading{value, std::time(0)};
                    }
                }
            });
    }

  private:
    const std::string metricId_;
    const std::string metadata_;
    const std::vector<std::shared_ptr<interfaces::Sensor>> sensors_;
    std::vector<std::optional<Reading>> readings_{sensors_.size()};
    const OperationType type_;

    Metric(const Metric&) = delete;
    Metric& operator=(const Metric&) = delete;
};
} // namespace core
