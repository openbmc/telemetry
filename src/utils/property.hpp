#pragma once

#include "dbus_object.hpp"
#include "on_change.hpp"
#include "property_validator.hpp"

#include <sdbusplus/asio/object_server.hpp>

namespace utils
{

template <class T>
class Property :
    public DbusObject,
    public OnChange<Property<T>>,
    public PropertyValidator<T>
{
  public:
    Property(
        const std::shared_ptr<sdbusplus::asio::dbus_interface>& interfaceArg,
        const std::string& nameArg, const T& valueArg,
        sdbusplus::asio::PropertyPermission permissionArg) :
        weakInterface(interfaceArg),
        name(nameArg), value(valueArg), permission(permissionArg)
    {
        register_property(*interfaceArg);
    }
    Property(const Property&) = delete;
    Property(Property&&) = delete;
    Property& operator=(const Property&) = delete;
    Property& operator=(Property&&) = delete;

    T get() const
    {
        return value;
    }

    void set(const T& newValue)
    {
        if (value != newValue)
        {
            if (auto interface = weakInterface.lock())
            {
                interface->set_property(name, newValue);

                if (permission == sdbusplus::asio::PropertyPermission::readOnly)
                {
                    this->changed();
                }
            }
        }
    }

  private:
    void register_property(sdbusplus::asio::dbus_interface& interface)
    {
        switch (permission)
        {
            case sdbusplus::asio::PropertyPermission::readOnly:
            {
                interface.register_property(name, value, permission);
                break;
            }
            case sdbusplus::asio::PropertyPermission::readWrite:
            {
                interface.register_property(
                    name, value,
                    [this](const T& newValue, T& peoperty) {
                        if (this->validate(newValue))
                        {
                            value = peoperty = newValue;
                            this->changed();
                            return true;
                        }

                        return false;
                    },
                    [](const T& peoperty) -> T { return peoperty; });
                break;
            }
        }
    }

    std::weak_ptr<sdbusplus::asio::dbus_interface> weakInterface;
    std::string name;
    T value;
    sdbusplus::asio::PropertyPermission permission;
};

template <class T>
class PropertyView : public DbusObject
{
  public:
    virtual ~PropertyView() = default;

    virtual T get() const = 0;
    virtual void update() const = 0;
};

namespace details
{

template <class T, class Getter>
class PropertyView : public utils::PropertyView<T>
{
  public:
    PropertyView(
        const std::shared_ptr<sdbusplus::asio::dbus_interface>& interfaceArg,
        const std::string& nameArg, Getter&& getter) :
        weakInterface(interfaceArg),
        name(nameArg), getter(std::forward<Getter>(getter))
    {
        register_property(*interfaceArg);
    }
    PropertyView(const PropertyView&) = delete;
    PropertyView(PropertyView&&) = delete;
    PropertyView& operator=(const PropertyView&) = delete;
    PropertyView& operator=(PropertyView&&) = delete;

    T get() const override
    {
        return getter();
    }

    void update() const override
    {
        if (auto interface = weakInterface.lock())
        {
            interface->set_property(name, getter());
        }
    }

  private:
    void register_property(sdbusplus::asio::dbus_interface& interface)
    {
        interface.register_property(
            name, getter(), sdbusplus::asio::PropertyPermission::readWrite);
    }

    std::weak_ptr<sdbusplus::asio::dbus_interface> weakInterface;
    std::string name;
    Getter getter;
};

} // namespace details

template <class T>
using PropertyViewPtr = std::shared_ptr<PropertyView<T>>;

template <class T>
using ConstPropertyViewPtr = std::shared_ptr<const PropertyView<T>>;

template <class T>
using PropertyPtr = std::shared_ptr<Property<T>>;

template <class T>
using ConstPropertyPtr = std::shared_ptr<const Property<T>>;

} // namespace utils
