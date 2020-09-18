#pragma once

#include <sdbusplus/asio/object_server.hpp>

#include <chrono>
#include <memory>
#include <vector>

class ReportManager
{
  public:
    ReportManager(
        const std::shared_ptr<sdbusplus::asio::object_server>& objServer);
    ~ReportManager();

    ReportManager(const ReportManager&) = delete;
    ReportManager& operator=(const ReportManager&) = delete;

  private:
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    std::shared_ptr<sdbusplus::asio::dbus_interface> reportManagerIntf;

    static constexpr uint32_t maxReports{20};
    static constexpr std::chrono::milliseconds pollRateResolution{1000};
};
