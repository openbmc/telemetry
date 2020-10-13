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

bool DbusEnvironment::synchronizeIoc(std::chrono::milliseconds timeout)
{
    auto promise = std::make_unique<std::promise<bool>>();
    auto future = promise->get_future();

    boost::asio::post(
        ioc, [promise = std::move(promise)] { promise->set_value(true); });

    if (future.wait_for(timeout) == std::future_status::ready)
    {
        return future.get();
    }

    return false;
}

std::function<void()> DbusEnvironment::setPromise(std::string_view name)
{
    auto promise = std::make_shared<std::promise<bool>>();

    {
        std::unique_lock<std::mutex> lock(mutex);
        futures[std::string(name)].emplace_back(promise->get_future());
    }

    return [p = std::move(promise)]() { p->set_value(true); };
}

bool DbusEnvironment::waitForFuture(std::string_view name,
                                    std::chrono::milliseconds timeout)
{
    if (auto future = getFuture(name); future.valid())
    {
        try
        {
            if (future.wait_for(timeout) == std::future_status::ready)
            {
                return future.get();
            }
        }
        catch (const std::future_error& e)
        {
            std::cerr << e.what() << "\n";
        }
    }

    return false;
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

std::pair<boost::system::error_code, std::string>
    DbusEnvironment::addReport(const AddReportArgs& args)
{
    std::promise<std::pair<boost::system::error_code, std::string>>
        addReportPromise;
    getBus()->async_method_call(
        [&addReportPromise](boost::system::error_code ec,
                            const std::string& path) {
            addReportPromise.set_value({ec, path});
        },
        serviceName(), reportManagerPath, reportManagerIfaceName, "AddReport",
        args.name, args.reportingType, args.emitsReadingsSignal,
        args.logToMetricReportsCollection, args.interval, args.readingParams);
    return addReportPromise.get_future().get();
}

boost::system::error_code DbusEnvironment::deleteReport(const std::string& path)
{
    std::promise<boost::system::error_code> deleteReportPromise;
    getBus()->async_method_call(
        [&deleteReportPromise](boost::system::error_code ec) {
            deleteReportPromise.set_value(ec);
        },
        serviceName(), path, deleteIfaceName, "Delete");
    return deleteReportPromise.get_future().get();
}

boost::asio::io_context DbusEnvironment::ioc;
std::shared_ptr<sdbusplus::asio::connection> DbusEnvironment::bus;
std::shared_ptr<sdbusplus::asio::object_server> DbusEnvironment::objServer;
std::mutex DbusEnvironment::mutex;
std::map<std::string, std::vector<std::future<bool>>> DbusEnvironment::futures;
