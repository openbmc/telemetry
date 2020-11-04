#pragma once

#include "interfaces/json_storage.hpp"
#include "interfaces/report.hpp"
#include "interfaces/report_factory.hpp"
#include "interfaces/report_manager.hpp"

#include <sdbusplus/asio/object_server.hpp>

#include <chrono>
#include <memory>
#include <vector>

class ReportManager : public interfaces::ReportManager
{
  public:
    ReportManager(
        boost::asio::io_context&,
        std::unique_ptr<interfaces::ReportFactory> reportFactory,
        std::unique_ptr<interfaces::JsonStorage> reportStorage,
        const std::shared_ptr<sdbusplus::asio::object_server>& objServer);
    ~ReportManager() = default;

    ReportManager(ReportManager&) = delete;
    ReportManager(ReportManager&&) = delete;
    ReportManager& operator=(ReportManager&) = delete;
    ReportManager& operator=(ReportManager&&) = delete;

    void removeReport(const interfaces::Report* report) override;
    static bool verifyScanPeriod(const uint64_t scanPeriod);

  private:
    std::unique_ptr<interfaces::ReportFactory> reportFactory;
    std::unique_ptr<interfaces::JsonStorage> reportStorage;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    std::unique_ptr<sdbusplus::asio::dbus_interface> reportManagerIface;
    std::vector<std::unique_ptr<interfaces::Report>> reports;

    std::unique_ptr<interfaces::Report>& addReport(
        boost::asio::yield_context& yield, const std::string& reportName,
        const std::string& reportingType, const bool emitsReadingsUpdate,
        const bool logToMetricReportsCollection, const uint64_t interval,
        const ReadingParameters& metricParams);
    void loadFromPersistent(boost::asio::yield_context& yield);

  public:
    static constexpr uint32_t maxReports{20};
    static constexpr std::chrono::milliseconds minInterval{1000};
    static constexpr const char* reportManagerIfaceName =
        "xyz.openbmc_project.Telemetry.ReportManager";
    static constexpr const char* reportManagerPath =
        "/xyz/openbmc_project/Telemetry/Reports";
};
