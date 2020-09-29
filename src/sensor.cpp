#include "sensor.hpp"

#include <phosphor-logging/log.hpp>

#include <functional>

Sensor::Sensor(interfaces::Sensor::Id sensorId, boost::asio::io_context& ioc,
               const std::shared_ptr<sdbusplus::asio::connection>& bus) :
    sensorId(std::move(sensorId)),
    ioc(ioc), bus(std::move(bus))
{}

Sensor::~Sensor() = default;

Sensor::Id Sensor::makeId(std::string_view service, std::string_view path)
{
    return Id("Sensor", service, path);
}

Sensor::Id Sensor::id() const
{
    return sensorId;
}

void Sensor::async_read()
{
    // prevents another async_read when previous did not finish
    uniqueCall([this](auto u) { async_read(std::move(u)); });
}

void Sensor::schedule(std::chrono::milliseconds interval)
{
    timerInterval = interval;

    if (!timer)
    {
        timer.emplace(ioc);
        timer->expires_after(timerInterval);
        timer->async_wait(
            [weakSelf = weak_from_this()](boost::system::error_code error) {
                Sensor::timerProc(error, weakSelf);
            });
    }
}

void Sensor::stop()
{
    timer = std::nullopt;
}

void Sensor::timerProc(boost::system::error_code error,
                       const std::weak_ptr<Sensor>& weakSelf)
{
    if (error)
    {
        return;
    }

    if (auto self = weakSelf.lock())
    {
        self->async_read();

        if (self->timer)
        {
            self->timer->expires_after(self->timerInterval);
            self->timer->async_wait(
                [weakSelf](boost::system::error_code error) {
                    Sensor::timerProc(error, weakSelf);
                });
        }
    }
}

void Sensor::async_read(utils::UniqueCall::Lock lock)
{
    bus->async_method_call(
        [weakSelf = weak_from_this(),
         id = sensorId](const boost::system::error_code& e,
                        const ValueVariant& valueVariant) {
            if (e)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "DBus 'GetProperty' call failed on Sensor Value",
                    phosphor::logging::entry("sensor=%s, ec=%lu",
                                             id.str().c_str(), e.value()));
                return;
            }

            auto value = readValue(valueVariant);
            if (!value)
            {
                phosphor::logging::log<phosphor::logging::level::ERR>(
                    "Unexpected 'Value' type",
                    phosphor::logging::entry("sensor=%s", id.str().c_str()));
                return;
            }

            if (auto self = weakSelf.lock())
            {
                if (self->value == value)
                {
                    for (const auto& weakListener : self->listeners)
                    {
                        if (auto listener = weakListener.lock())
                        {
                            listener->sensorUpdated(*self);
                        }
                    }
                }
                else
                {
                    self->value = value;

                    for (const auto& weakListener : self->listeners)
                    {
                        if (auto listener = weakListener.lock())
                        {
                            listener->sensorUpdated(*self, *self->value);
                        }
                    }
                }
            }
        },
        sensorId.service, sensorId.path, "org.freedesktop.DBus.Properties",
        "Get", "xyz.openbmc_project.Sensor.Value", "Value");
}

void Sensor::registerForUpdates(
    const std::weak_ptr<interfaces::SensorListener>& weakListener)
{
    listeners.emplace_back(weakListener);

    if (auto listener = weakListener.lock())
    {
        if (value)
        {
            listener->sensorUpdated(*this, *value);
        }
        else
        {
            listener->sensorUpdated(*this);
        }
    }
}

std::optional<double> Sensor::readValue(const ValueVariant& v)
{
    if (auto d = std::get_if<double>(&v))
    {
        return *d;
    }
    else if (auto i = std::get_if<int64_t>(&v))
    {
        return static_cast<double>(*i);
    }
    else
    {
        return std::nullopt;
    }
}
