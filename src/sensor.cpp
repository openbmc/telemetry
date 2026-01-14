#include "sensor.hpp"

#include "utils/clock.hpp"

#include <boost/container/flat_map.hpp>
#include <phosphor-logging/log.hpp>
#include <sdbusplus/asio/property.hpp>
#include <xyz/openbmc_project/Sensor/Value/common.hpp>

#include <functional>

using SensorValue = sdbusplus::common::xyz::openbmc_project::sensor::Value;

Sensor::Sensor(interfaces::Sensor::Id sensorId,
               const std::string& sensorMetadata, boost::asio::io_context& ioc,
               const std::shared_ptr<sdbusplus::asio::connection>& bus) :
    sensorId(std::move(sensorId)), sensorMetadata(sensorMetadata), ioc(ioc),
    bus(bus)
{}

Sensor::Id Sensor::makeId(std::string_view service, std::string_view path)
{
    return Id("Sensor", service, path);
}

Sensor::Id Sensor::id() const
{
    return sensorId;
}

std::string Sensor::metadata() const
{
    return sensorMetadata;
}

std::string Sensor::getName() const
{
    return sensorMetadata.empty() ? sensorId.path : sensorMetadata;
}

void Sensor::async_read()
{
    uniqueCall([this](auto lock) { async_read(std::move(lock)); });
}

void Sensor::async_read(std::shared_ptr<utils::UniqueCall::Lock> lock)
{
    makeSignalMonitor();

    sdbusplus::asio::getProperty<double>(
        *bus, sensorId.service, sensorId.path, SensorValue::interface,
        SensorValue::property_names::value,
        [lock, id = sensorId, weakSelf = weak_from_this()](
            boost::system::error_code ec, double newValue) {
            if (ec)
            {
                phosphor::logging::log<phosphor::logging::level::WARNING>(
                    "DBus 'GetProperty' call failed on Sensor Value",
                    phosphor::logging::entry("SENSOR_PATH=%s", id.path.c_str()),
                    phosphor::logging::entry("ERROR_CODE=%d", ec.value()));
                return;
            }
            if (auto self = weakSelf.lock())
            {
                self->updateValue(newValue);
            }
        });
}

void Sensor::registerForUpdates(
    const std::weak_ptr<interfaces::SensorListener>& weakListener)
{
    listeners.erase(
        std::remove_if(listeners.begin(), listeners.end(),
                       [](const auto& listener) { return listener.expired(); }),
        listeners.end());

    if (auto listener = weakListener.lock())
    {
        listeners.emplace_back(weakListener);

        if (value)
        {
            listener->sensorUpdated(*this, timestamp, *value);
        }
        else
        {
            async_read();
        }
    }
}

void Sensor::unregisterFromUpdates(
    const std::weak_ptr<interfaces::SensorListener>& weakListener)
{
    if (auto listener = weakListener.lock())
    {
        listeners.erase(
            std::remove_if(
                listeners.begin(), listeners.end(),
                [listenerToUnregister = listener.get()](const auto& listener) {
                    return (listener.expired() ||
                            listener.lock().get() == listenerToUnregister);
                }),
            listeners.end());
    }
}

void Sensor::updateValue(double newValue)
{
    timestamp = Clock().steadyTimestamp();

    if (value != newValue)
    {
        value = newValue;

        for (const auto& weakListener : listeners)
        {
            if (auto listener = weakListener.lock())
            {
                listener->sensorUpdated(*this, timestamp, *value);
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

    const auto param =
        "type='signal',member='PropertiesChanged',path='"s + sensorId.path +
        "',arg0='" + SensorValue::interface + "'"s;

    signalMonitor = std::make_unique<sdbusplus::bus::match_t>(
        *bus, param,
        [weakSelf = weak_from_this()](sdbusplus::message_t& message) {
            signalProc(weakSelf, message);
        });
}

void Sensor::signalProc(const std::weak_ptr<Sensor>& weakSelf,
                        sdbusplus::message_t& message)
{
    if (auto self = weakSelf.lock())
    {
        std::string iface;
        boost::container::flat_map<std::string, ValueVariant>
            changed_properties;
        std::vector<std::string> invalidated_properties;

        message.read(iface, changed_properties, invalidated_properties);

        if (iface == SensorValue::interface)
        {
            const auto it =
                changed_properties.find(SensorValue::property_names::value);
            if (it != changed_properties.end())
            {
                if (auto val = std::get_if<double>(&it->second))
                {
                    self->updateValue(*val);
                }
                else
                {
                    phosphor::logging::log<phosphor::logging::level::ERR>(
                        "Failed to receive Value from Sensor "
                        "PropertiesChanged signal",
                        phosphor::logging::entry("SENSOR_PATH=%s",
                                                 self->sensorId.path.c_str()));
                }
            }
        }
    }
}

LabeledSensorInfo Sensor::getLabeledSensorInfo() const
{
    return LabeledSensorInfo(sensorId.service, sensorId.path, sensorMetadata);
}
