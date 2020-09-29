#include <boost/asio.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>

namespace stubs
{

class SensorService
{
  public:
    SensorService(
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

    ~SensorService()
    {
        objServer->remove_interface(sensorIface);
    }

    static const char* path()
    {
        return "/telemetry/ut/SensorService";
    }

    static const char* interface()
    {
        return "xyz.openbmc_project.Sensor.Value";
    }

    struct Properties
    {
        static const char* value()
        {
            return "Value";
        }
    };

    static constexpr Properties property = {};

    double value = 0.0;

  private:
    boost::asio::io_context& ioc;
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;

    std::shared_ptr<sdbusplus::asio::dbus_interface> sensorIface;
};

} // namespace stubs
