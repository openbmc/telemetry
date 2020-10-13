#pragma once

#include "interfaces/report.hpp"
#include "interfaces/report_manager.hpp"
#include "telemetry/types.hpp"

#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <chrono>
#include <memory>

class Report : public interfaces::Report
{
  public:
    Report(boost::asio::io_context& ioc,
           const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
           const std::string& reportName, const std::string& reportingType,
           const bool emitsReadingsSignal,
           const bool logToMetricReportsCollection,
           const std::chrono::milliseconds period,
           const ReadingParameters& metricParams,
           interfaces::ReportManager& reportManager);
    ~Report() = default;

    Report(Report&) = delete;
    Report(Report&&) = delete;
    Report& operator=(Report&) = delete;
    Report& operator=(Report&&) = delete;

    std::string getName() const override
    {
        return name;
    }

    std::string getPath() const override
    {
        return path;
    }

  private:
    const std::string name;
    const std::string path;
    std::chrono::milliseconds interval;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    std::unique_ptr<sdbusplus::asio::dbus_interface> reportIface;
    std::unique_ptr<sdbusplus::asio::dbus_interface> deleteIface;
};
