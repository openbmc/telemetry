#include "app.hpp"
#include "utils/log.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <sdbusplus/asio/connection.hpp>

#include <iostream>
#include <memory>

int main()
{
    utils::Logger::setLogLevel(utils::LogLevel::Info);

    boost::asio::io_context ioc;
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);

    std::shared_ptr<sdbusplus::asio::connection> bus =
        std::make_shared<sdbusplus::asio::connection>(ioc);

    const char* serviceName = "xyz.openbmc_project.MonitoringService";
    bus->request_name(serviceName);

    signals.async_wait(
        [&ioc](const boost::system::error_code ec, const int& sig) {
            if (ec)
            {
                LOG_ERROR << "signal_handler - error:" << ec;
            }

            LOG_ERROR << "signal_handler - signal: " << sig;

            ioc.stop();
        });

    LOG_INFO << "Application starting";
    App app(bus);

    ioc.run();

    return 0;
}
