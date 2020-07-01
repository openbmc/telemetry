#pragma once

#include "dbus/dbus.hpp"
#include "utils/property.hpp"
#include "utils/scoped_resource.hpp"
#include "utils/signal.hpp"
#include "utils/utils.hpp"

namespace utils
{

class DbusInterfaces
{
  public:
    DbusInterfaces(const DbusInterfaces&) = delete;
    DbusInterfaces(DbusInterfaces&&) = delete;
    DbusInterfaces& operator=(const DbusInterfaces&) = delete;
    DbusInterfaces& operator=(DbusInterfaces&&) = delete;

    DbusInterfaces(const std::shared_ptr<sdbusplus::asio::object_server>&
                       objectServerArg) :
        objectServer(objectServerArg)
    {}

    template <class F>
    std::shared_ptr<sdbusplus::asio::dbus_interface>
        addInterface(const std::string& path, const std::string& name,
                     F&& function)
    {
        try
        {
            auto& interface = dbusInterfaces.emplace_back(
                objectServer->add_interface(path, name),
                DbusInterfaceCleaner(objectServer));
            function(**interface);
            (*interface)->initialize();

            return *interface;
        }
        catch (const std::exception& e)
        {
            throwError(std::errc::invalid_argument, e.what());
        }
    }

    template <class T>
    PropertyPtr<T> make_property_r(sdbusplus::asio::dbus_interface& interface,
                                   const std::string& name, const T& value)
    {
        return add_object(std::make_shared<Property<T>>(
            interface, name, value,
            sdbusplus::asio::PropertyPermission::readOnly));
    }

    template <class T>
    PropertyPtr<T> make_property_rw(sdbusplus::asio::dbus_interface& interface,
                                    const std::string& name, const T& value)
    {
        return add_object(std::make_shared<Property<T>>(
            interface, name, value,
            sdbusplus::asio::PropertyPermission::readWrite));
    }

    template <class Getter>
    auto make_property_view(sdbusplus::asio::dbus_interface& interface,
                            const std::string& name, Getter&& getter)
    {
        using T = std::decay_t<decltype(getter())>;
        return add_object(std::make_shared<details::PropertyView<T, Getter>>(
            interface, name, std::forward<Getter>(getter)));
    }

    template <class... Args>
    SignalPtr<Args...>
        make_signal(const std::shared_ptr<sdbusplus::asio::connection>& bus,
                    sdbusplus::asio::dbus_interface& interface,
                    const std::string& name)
    {
        return add_object(
            std::make_shared<Signal<Args...>>(bus, interface, name));
    }

  private:
    template <class T>
    std::shared_ptr<T> add_object(const std::shared_ptr<T>& property)
    {
        objects.emplace_back(property);
        return property;
    }

    std::shared_ptr<sdbusplus::asio::object_server> objectServer;
    std::vector<DbusScopedInterface> dbusInterfaces;
    std::vector<std::shared_ptr<DbusObject>> objects;
};

} // namespace utils
