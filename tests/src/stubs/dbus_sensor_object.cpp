#include "dbus_sensor_object.hpp"

#include "helpers.hpp"

#include <boost/asio.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>

namespace stubs
{

DbusSensorObject::DbusSensorObject(
    boost::asio::io_context& ioc,
    const std::shared_ptr<sdbusplus::asio::connection>& bus,
    const std::shared_ptr<sdbusplus::asio::object_server>& objServer) :
    ioc(ioc),
    bus(bus), objServer(objServer)
{
    sensorIface = objServer->add_unique_interface(
        path(), interface(), [this](auto& iface) {
            iface.register_property_r(
                property.value(), value,
                sdbusplus::vtable::property_::emits_change,
                [this](const auto&) { return value; });
        });
}

void DbusSensorObject::setValue(double v)
{
    value = v;

    sensorIface->signal_property(property.value());
}

double DbusSensorObject::getValue() const
{
    return value;
}

const char* DbusSensorObject::path()
{
    return "/telemetry/ut/DbusSensorObject";
}

const char* DbusSensorObject::interface()
{
    return "xyz.openbmc_project.Sensor.Value";
}

const char* DbusSensorObject::Properties::value()
{
    return "Value";
}

} // namespace stubs
