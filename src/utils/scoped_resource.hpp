#pragma once

#include <memory>
#include <optional>
#include <utility>

namespace utils
{

class DbusInterfaceCleaner
{
  public:
    explicit DbusInterfaceCleaner(
        const std::shared_ptr<sdbusplus::asio::object_server>& serverArg) :
        server(serverArg)
    {}

    void operator()(std::shared_ptr<sdbusplus::asio::dbus_interface>& ifce)
    {
        server->remove_interface(ifce);
        ifce = nullptr;
    }

  private:
    std::shared_ptr<sdbusplus::asio::object_server> server;
};

template <class T, class Cleaner>
class ScopedResource
{
  public:
    ScopedResource() = default;
    ScopedResource(T resourceArg, Cleaner cleanerArg) :
        resource(std::move(resourceArg)), cleaner(std::move(cleanerArg))
    {}
    ScopedResource(const ScopedResource&) = delete;
    ScopedResource(ScopedResource&& other) :
        resource(std::move(other.resource)), cleaner(std::move(other.cleaner))
    {
        other.resource = std::nullopt;
        other.cleaner = std::nullopt;
    }

    ScopedResource& operator=(const ScopedResource&) = delete;
    ScopedResource& operator=(ScopedResource&& other)
    {
        clear();

        std::swap(cleaner, other.cleaner);
        std::swap(resource, other.resource);

        return *this;
    }

    ~ScopedResource()
    {
        clear();
    }

    T& operator*()
    {
        return resource.value();
    }

    const T& operator*() const
    {
        return resource.value();
    }

    T* operator->()
    {
        return &resource.value();
    }

    const T* operator->() const
    {
        return &resource.value();
    }

  private:
    void clear()
    {
        if (cleaner and resource)
        {
            (*cleaner)(*resource);
        }

        cleaner = std::nullopt;
        resource = std::nullopt;
    }

    std::optional<T> resource;
    std::optional<Cleaner> cleaner;
};

using DbusScopedInterface =
    ScopedResource<std::shared_ptr<sdbusplus::asio::dbus_interface>,
                   DbusInterfaceCleaner>;

} // namespace utils
