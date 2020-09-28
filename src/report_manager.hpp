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

    ReportManager(const ReportManager&) = delete;
    ReportManager& operator=(const ReportManager&) = delete;

    static bool verifyScanPeriod(const uint64_t scanPeriod);

  private:
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    std::shared_ptr<sdbusplus::asio::dbus_interface> reportManagerIntf;
    std::vector<std::shared_ptr<Report>> reports;

    static constexpr uint32_t maxReports{20};
    static constexpr std::chrono::milliseconds pollRateResolution{1000};

    static_assert(pollRateResolution > std::chrono::milliseconds::zero());

    auto getRemoveReportCallback();
};
