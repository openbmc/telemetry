#pragma once

#include "core/interfaces/report.hpp"
#include "dbus/dbus.hpp"
#include "dbus/report.hpp"

#include <sdbusplus/asio/object_server.hpp>

namespace dbus::interfaces
{
class ReportFactory
{
  public:
    virtual ~ReportFactory() = default;

    virtual std::tuple<std::shared_ptr<core::interfaces::Report>,
                       std::shared_ptr<dbus::Report>>
        make(boost::asio::yield_context& yield,
             std::shared_ptr<sdbusplus::asio::connection> bus,
             const core::ReportName&, const std::string& reportingType,
             const std::vector<std::string>& reportAction,
             const uint32_t scanPeriod,
             const std::vector<MetricParams>& metricParams,
             std::shared_ptr<sdbusplus::asio::object_server> objServer)
            const = 0;
};
} // namespace dbus::interfaces
