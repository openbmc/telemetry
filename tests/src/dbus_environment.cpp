#include "dbus_environment.hpp"

#include <future>
#include <thread>

void DbusEnvironment::SetUp()
{
    bus = std::make_shared<sdbusplus::asio::connection>(ioc);
    bus->request_name(serviceName());

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

const char* DbusEnvironment::serviceName()
{
    return "telemetry.ut";
}

void DbusEnvironment::synchronizeIoc()
{
    std::promise<bool> promise;

    boost::asio::post(ioc, [&promise] { promise.set_value(true); });

    promise.get_future().wait_for(std::chrono::seconds(5));
}

std::function<void()> DbusEnvironment::setPromise(std::string_view name)
{
    std::shared_ptr<std::promise<bool>> promise =
        std::make_shared<std::promise<bool>>();

    {
        std::unique_lock<std::mutex> lock(mutex);
        futures[std::string(name)].emplace_back(promise->get_future());
    }

    return [p = std::move(promise)]() { p->set_value(true); };
}

void DbusEnvironment::waitForFuture(std::string_view name)
{
    auto future = getFuture(name);

    if (future.valid())
    {
        try
        {
            future.wait_for(std::chrono::seconds(5));
        }
        catch (const std::future_error& e)
        {
            std::cerr << e.what() << "\n";
        }
    }
}

std::future<bool> DbusEnvironment::getFuture(std::string_view name)
{
    std::unique_lock<std::mutex> lock(mutex);

    auto& data = futures[std::string(name)];
    auto it = std::find_if(data.begin(), data.end(),
                           [](const auto& f) { return f.valid(); });

    if (it != data.end())
    {
        auto result = std::move(*it);
        data.erase(it);
        return result;
    }

    return {};
}

boost::asio::io_context DbusEnvironment::ioc;
std::shared_ptr<sdbusplus::asio::connection> DbusEnvironment::bus;
std::shared_ptr<sdbusplus::asio::object_server> DbusEnvironment::objServer;
std::mutex DbusEnvironment::mutex;
std::map<std::string, std::vector<std::future<bool>>> DbusEnvironment::futures;
