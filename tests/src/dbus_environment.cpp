#include "dbus_environment.hpp"

#include <thread>

void DbusEnvironment::SetUp()
{
    bus = std::make_shared<sdbusplus::asio::connection>(ioc);
    bus->request_name(service());

    objServer = std::make_unique<sdbusplus::asio::object_server>(bus);

    iocThread = std::thread([this] { ioc.run(); });
}

void DbusEnvironment::TearDown()
{
    ioc.stop();
    iocThread.join();

    objServer = nullptr;
    bus = nullptr;
}

boost::asio::io_context& DbusEnvironment::getIoc()
{
    return ioc;
}

std::shared_ptr<sdbusplus::asio::connection> DbusEnvironment::getBus()
{
    return bus;
}

std::shared_ptr<sdbusplus::asio::object_server> DbusEnvironment::getObjServer()
{
    return objServer;
}

const char* DbusEnvironment::service()
{
    return "telemetry.ut";
}

void DbusEnvironment::sleep(std::chrono::milliseconds time)
{
    std::this_thread::sleep_for(time);
}

boost::asio::io_context DbusEnvironment::ioc;
std::shared_ptr<sdbusplus::asio::connection> DbusEnvironment::bus;
std::shared_ptr<sdbusplus::asio::object_server> DbusEnvironment::objServer;
