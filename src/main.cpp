#include "dbus/dbus.hpp"
#include "dbus/report_mgr.hpp"
#include "log.hpp"

#include <boost/asio.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <iostream>
#include <memory>

core::SensorCache::AllSensors core::SensorCache::sensors;

class App
{
  public:
    App(boost::asio::io_context& ioc) :
        ioc(ioc), bus{std::make_shared<typeof(*bus)>(ioc)},
        objServer{std::make_shared<typeof(*objServer)>(bus)},
        objManager{std::make_shared<typeof(*objManager)>(*bus, dbus::Path)},
        reportManager{dbus::ReportManager::make(bus, objServer)}
    {}

  private:
    std::reference_wrapper<boost::asio::io_context> ioc;
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    std::shared_ptr<sdbusplus::server::manager::manager> objManager;
    std::shared_ptr<dbus::ReportManager> reportManager;
}; // namespace App

int main()
{
    utils::Logger::setLogLevel(utils::LogLevel::debug);

    boost::asio::io_context ioc;
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);

    signals.async_wait(
        [&ioc](const boost::system::error_code&, const int&) { ioc.stop(); });

    LOG_INFO << "Application starting";

    App app(ioc);

    ioc.run();

    return 0;
}
