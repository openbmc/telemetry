#pragma once

#include "utils/dbus_interface.hpp"

#include <sdbusplus/asio/object_server.hpp>

#include <chrono>
#include <memory>
#include <vector>

class ReportManager
{
  public:
    ReportManager(
        const std::shared_ptr<sdbusplus::asio::object_server>& objServer);

    ReportManager(const ReportManager&) = delete;
    ReportManager& operator=(const ReportManager&) = delete;

  private:
    utils::DbusInterface reportManagerIntf;

    static constexpr uint32_t maxReports{20};
    static constexpr std::chrono::milliseconds pollRateResolution{1000};

    static_assert(pollRateResolution > std::chrono::milliseconds::zero());
};
