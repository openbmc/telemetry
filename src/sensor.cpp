#include "sensor.hpp"

#include "utils/detached_timer.hpp"

#include <boost/container/flat_map.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/bus/match.hpp>

#include <functional>
#include <iostream>

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
    uniqueCall([this](auto context) { async_read(std::move(context)); });
}

void Sensor::async_read(std::shared_ptr<utils::UniqueCall::Lock> context)
{
    sdbusplus::asio::getProperty<double>(
        *bus, sensorId.service, sensorId.path,
        "xyz.openbmc_project.Sensor.Value", "Value",
        [context, id = sensorId,
         weakSelf = weak_from_this()](boost::system::error_code ec) {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "DBus 'GetProperty' call failed on Sensor Value",
                phosphor::logging::entry("sensor=%s, ec=%lu", id.str().c_str(),
                                         ec.value()));

            if (auto self = weakSelf.lock())
            {
                self->makeSignalMonitor();

                utils::makeDetachedTimer(self->ioc, std::chrono::seconds(30),
                                         [weakSelf] {
                                             if (auto self = weakSelf.lock())
                                             {
                                                 self->async_read();
                                             }
                                         });
            }
        },
        [context, weakSelf = weak_from_this()](double newValue) {
            if (auto self = weakSelf.lock())
            {
                self->makeSignalMonitor();
                self->updateValue(newValue);
            }
        });
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
            async_read();
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

void Sensor::updateValue()
{
    for (const auto& weakListener : listeners)
    {
        if (auto listener = weakListener.lock())
        {
            listener->sensorUpdated(*this);
        }
    }
}

void Sensor::updateValue(double newValue)
{
    if (value == newValue)
    {
        updateValue();
    }
    else
    {
        value = newValue;

        for (const auto& weakListener : listeners)
        {
            if (auto listener = weakListener.lock())
            {
                listener->sensorUpdated(*this, *value);
            }
        }
    }
}

void Sensor::makeSignalMonitor()
{
    if (signalMonitor)
    {
        return;
    }

    using namespace std::string_literals;

    const auto param = "type='signal',member='PropertiesChanged',path='"s +
                       sensorId.path + "',"s +
                       "arg0='xyz.openbmc_project.Sensor.Value'"s;

    signalMonitor = std::make_unique<sdbusplus::bus::match::match>(
        *bus, param,
        [weakSelf = weak_from_this()](sdbusplus::message::message& message) {
            signalProc(weakSelf, message);
        });
}

void Sensor::signalProc(const std::weak_ptr<Sensor>& weakSelf,
                        sdbusplus::message::message& message)
{
    try
    {
        if (auto self = weakSelf.lock())
        {
            std::string iface;
            boost::container::flat_map<std::string, ValueVariant>
                changed_properties;
            std::vector<std::string> invalidated_properties;

            message.read(iface, changed_properties, invalidated_properties);

            if (iface == "xyz.openbmc_project.Sensor.Value")
            {
                const auto it = changed_properties.find("Value");
                if (it != changed_properties.end())
                {
                    if (auto val = readValue(it->second))
                    {
                        self->updateValue(*val);
                    }
                    else
                    {
                        phosphor::logging::log<phosphor::logging::level::ERR>(
                            "Failed to receive Value from Sensor "
                            "PropertiesChanged signal",
                            phosphor::logging::entry(
                                "sensor=%s", self->sensorId.path.c_str()));
                    }
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Sensor::signalProc ",
            phosphor::logging::entry("error=%s", e.what()));
    }
}
