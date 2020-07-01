#pragma once

#include "dbus_object.hpp"

#include <sdbusplus/asio/object_server.hpp>

namespace utils
{

template <class... Args>
class Signal : public DbusObject
{
  public:
    Signal(const std::shared_ptr<sdbusplus::asio::connection>& busArg,
           const std::shared_ptr<sdbusplus::asio::dbus_interface>& interfaceArg,
           const std::string& nameArg) :
        bus(busArg),
        weakInterface(interfaceArg), name(nameArg)
    {
        interfaceArg->register_signal<Args...>(name);
    }

    void signal() const
    {
        if (auto interface = weakInterface.lock())
        {
            auto s = bus->new_signal(interface->get_object_path().c_str(),
                                     interface->get_interface_name().c_str(),
                                     name.c_str());
            s.signal_send();
        }
    }

  private:
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::weak_ptr<sdbusplus::asio::dbus_interface> weakInterface;
    std::string name;
};

template <class... Args>
using SignalPtr = std::shared_ptr<const Signal<Args...>>;

} // namespace utils
