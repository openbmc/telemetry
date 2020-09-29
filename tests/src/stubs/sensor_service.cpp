#include "sensor_service.hpp"

#include <boost/asio.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>

namespace stubs
{

SensorService::SensorService(
    boost::asio::io_context& ioc,
    const std::shared_ptr<sdbusplus::asio::connection>& bus,
    const std::shared_ptr<sdbusplus::asio::object_server>& objServer) :
    ioc(ioc),
    bus(bus), objServer(objServer)
{
    sensorIface = objServer->add_interface(path(), interface());

    sensorIface->register_property_r(property.value(), double{},
                                     sdbusplus::vtable::property_::const_,
                                     [this](const auto&) { return value; });

    sensorIface->initialize();
}

SensorService::~SensorService()
{
    objServer->remove_interface(sensorIface);
}

const char* SensorService::path()
{
    return "/telemetry/ut/SensorService";
}

const char* SensorService::interface()
{
    return "xyz.openbmc_project.Sensor.Value";
}

const char* SensorService::Properties::value()
{
    return "Value";
}

} // namespace stubs
