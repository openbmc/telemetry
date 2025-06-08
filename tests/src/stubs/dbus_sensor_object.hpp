#pragma once

#include <boost/asio.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>
#include <sdbusplus/bus.hpp>

namespace stubs
{

class DbusSensorObject
{
  public:
    DbusSensorObject(
        boost::asio::io_context& ioc,
        const std::shared_ptr<sdbusplus::asio::connection>& bus,
        const std::shared_ptr<sdbusplus::asio::object_server>& objServer);

    static const char* path();
    static const char* interface();

    void setValue(double);
    double getValue() const;

    struct Properties
    {
        static const char* value();
    };

    static constexpr Properties property = {};

  private:
    [[maybe_unused]] boost::asio::io_context& ioc;
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;

    std::unique_ptr<sdbusplus::asio::dbus_interface> sensorIface;

    double value = 0.0;
};

} // namespace stubs
