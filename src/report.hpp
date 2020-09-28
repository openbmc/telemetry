#pragma once

#include <sdbusplus/asio/object_server.hpp>

#include <chrono>
#include <memory>

using Readings = std::tuple<
    uint64_t,
    std::vector<std::tuple<std::string, std::string, double, uint64_t>>>;
using ReadingParameters =
    std::vector<std::tuple<std::vector<sdbusplus::message::object_path>,
                           std::string, std::string, std::string>>;

class Report : std::enable_shared_from_this<Report>
{
  public:
    Report(const std::shared_ptr<sdbusplus::asio::connection>& bus,
           const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
           const std::string& domain, const std::string& name,
           const std::string& type, const bool emitsReadingsSignal,
           const bool logToMetricReportsCollection,
           const std::chrono::milliseconds&& period,
           const ReadingParameters& metricParams,
           std::function<void(const std::shared_ptr<Report>&)> removeCallbac);
    ~Report();

    std::string path;

  private:
    std::chrono::milliseconds scanPeriod;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    std::shared_ptr<sdbusplus::asio::dbus_interface> reportIntf;
    std::shared_ptr<sdbusplus::asio::dbus_interface> deleteIface;
};
