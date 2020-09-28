#pragma once

#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <chrono>
#include <memory>

using Readings = std::tuple<
    uint64_t,
    std::vector<std::tuple<std::string, std::string, double, uint64_t>>>;
using ReadingParameters =
    std::vector<std::tuple<std::vector<sdbusplus::message::object_path>,
                           std::string, std::string, std::string>>;

class ReportManager;

class Report
{
  public:
    Report(boost::asio::io_context& ioc,
           const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
           const std::string& name, const std::string& reportingType,
           const bool emitsReadingsSignal,
           const bool logToMetricReportsCollection,
           const std::chrono::milliseconds period,
           const ReadingParameters& metricParams, ReportManager& reportManager);
    ~Report() = default;

    Report(Report&) = delete;
    Report(Report&&) = delete;
    Report& operator=(Report&) = delete;
    Report& operator=(Report&&) = delete;

    const std::string name;
    const std::string path;

  private:
    std::chrono::milliseconds interval;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    std::unique_ptr<sdbusplus::asio::dbus_interface> reportIface;
    std::unique_ptr<sdbusplus::asio::dbus_interface> deleteIface;
};
