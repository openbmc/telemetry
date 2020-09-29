#include "sensor.hpp"

#include "utils/log.hpp"

#include <functional>

Sensor::Sensor(interfaces::Sensor::Id sensorId,
               const std::shared_ptr<sdbusplus::asio::connection>& bus) :
    sensorId(std::move(sensorId)),
    bus(std::move(bus))
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
    bus->async_method_call(
        [weakSelf = weak_from_this(),
         id = sensorId](const boost::system::error_code& e,
                        const ValueVariant& valueVariant) {
            if (e)
            {
                LOG_ERROR_T(id) << "D-Bus 'Get' failed";
                return;
            }

            auto value = readValue(valueVariant);
            if (!value)
            {
                LOG_ERROR_T(id) << "Unexpected 'Value' type";
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
