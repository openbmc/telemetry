#pragma once

#include <sdbusplus/asio/object_server.hpp>

#include <memory>

namespace utils
{

class DbusInterface
{
  public:
    DbusInterface() = default;
    template <class Initializer>
    DbusInterface(
        const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
        const std::string& path, const std::string& name,
        Initializer&& initializer) :
        objServer(objServer)
    {
        interface = objServer->add_interface(path, name);

        initializer(interface);

        interface->initialize();
    }

    DbusInterface(const DbusInterface&) = delete;
    DbusInterface(DbusInterface&&) = default;

    ~DbusInterface()
    {
        if (objServer && interface)
        {
            objServer->remove_interface(interface);
        }
    }

    DbusInterface& operator=(const DbusInterface&) = delete;
    DbusInterface& operator=(DbusInterface&&) = default;

  private:
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    std::shared_ptr<sdbusplus::asio::dbus_interface> interface;
};

} // namespace utils
