#include "dbus/dbus.hpp"
#include "dbus/report_mgr.hpp"
#include "log.hpp"

#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <iostream>
#include <memory>

core::SensorCache::AllSensors core::SensorCache::sensors;

class App
{
  public:
    App(boost::asio::io_context& ioc,
        std::shared_ptr<sdbusplus::asio::connection> bus) :
        objServer(std::make_shared<sdbusplus::asio::object_server>(bus)),
        objManager(*bus, dbus::Path),
        reportManager(dbus::ReportManager::make(bus, objServer))
    {}

  private:
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    sdbusplus::server::manager::manager objManager;
    std::shared_ptr<dbus::ReportManager> reportManager;
};

int main()
{
    utils::Logger::setLogLevel(utils::LogLevel::info);

    boost::asio::io_context ioc;
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);

    std::shared_ptr<sdbusplus::asio::connection> bus =
        std::make_shared<sdbusplus::asio::connection>(ioc);

    signals.async_wait(
        [&ioc](const boost::system::error_code& ec, const int& sig) {
            if (ec)
            {
                LOG_ERROR << "signal_handler - error:" << rc;
            }

            LOG_ERROR << "signal_handler - signal: " << sig;

            ioc.stop();
        });

    LOG_INFO << "Application starting";

    App app(ioc, bus);

    ioc.run();

    return 0;
}
