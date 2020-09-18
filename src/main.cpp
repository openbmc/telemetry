#include "telemetry.hpp"
#include "utils/log.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <sdbusplus/asio/connection.hpp>

#include <memory>
#include <stdexcept>

int main()
{
    boost::asio::io_context ioc;
    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);

    auto bus = std::make_shared<sdbusplus::asio::connection>(ioc);

    constexpr const char* serviceName = "xyz.openbmc_project.Telemetry";
    bus->request_name(serviceName);

    signals.async_wait(
        [&ioc](const boost::system::error_code ec, const int& sig) {
            if (ec)
            {
                LOG_ERROR << "signal_handler - error: " << ec;
                throw std::runtime_error("Signal should not be canceled");
            }

            ioc.stop();
        });

    LOG_INFO << "Telemetry starting";
    Telemetry app(bus);
    ioc.run();

    return 0;
}
