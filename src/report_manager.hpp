#pragma once

#include "report.hpp"

#include <sdbusplus/asio/object_server.hpp>

#include <chrono>
#include <memory>
#include <vector>

class ReportManager
{
  public:
    ReportManager(
        const std::shared_ptr<sdbusplus::asio::connection>& bus,
        const std::shared_ptr<sdbusplus::asio::object_server>& objServer);
    ~ReportManager();

    ReportManager(ReportManager&) = delete;
    ReportManager(ReportManager&&) = delete;
    ReportManager& operator=(ReportManager&) = delete;
    ReportManager& operator=(ReportManager&&) = delete;

    static bool verifyScanPeriod(const uint64_t scanPeriod);
    void removeReport(const Report* report);

  private:
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    std::shared_ptr<sdbusplus::asio::dbus_interface> reportManagerIntf;
    std::vector<std::unique_ptr<Report>> reports;

  public:
    static constexpr uint32_t maxReports{20};
    static constexpr std::chrono::milliseconds minInterval{1000};
};
