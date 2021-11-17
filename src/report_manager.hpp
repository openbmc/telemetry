#pragma once

#include "interfaces/json_storage.hpp"
#include "interfaces/report.hpp"
#include "interfaces/report_factory.hpp"
#include "interfaces/report_manager.hpp"
#include "report.hpp"

#include <systemd/sd-bus-protocol.h>

#include <sdbusplus/asio/object_server.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <vector>

class ReportManager : public interfaces::ReportManager
{
  public:
    ReportManager(
        std::unique_ptr<interfaces::ReportFactory> reportFactory,
        std::unique_ptr<interfaces::JsonStorage> reportStorage,
        const std::shared_ptr<sdbusplus::asio::object_server>& objServer);
    ~ReportManager() = default;

    ReportManager(ReportManager&) = delete;
    ReportManager(ReportManager&&) = delete;
    ReportManager& operator=(ReportManager&) = delete;
    ReportManager& operator=(ReportManager&&) = delete;

    void removeReport(const interfaces::Report* report) override;
    void updateReport(const std::string& name) override;

  private:
    std::unique_ptr<interfaces::ReportFactory> reportFactory;
    std::unique_ptr<interfaces::JsonStorage> reportStorage;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    std::unique_ptr<sdbusplus::asio::dbus_interface> reportManagerIface;
    std::vector<std::unique_ptr<interfaces::Report>> reports;

    void verifyReportNameLength(const std::string& reportName);
    void verifyAddReport(
        const std::string& reportName, const std::string& reportingType,
        Milliseconds interval, const std::string& reportUpdates,
        const std::vector<LabeledMetricParameters>& readingParams);
    interfaces::Report& addReport(
        boost::asio::yield_context& yield, const std::string& reportName,
        const std::string& reportingType, const bool emitsReadingsUpdate,
        const bool logToMetricReportsCollection, Milliseconds interval,
        const uint64_t appendLimit, const std::string& reportUpdates,
        ReadingParameters metricParams, const bool enabled);
    interfaces::Report& addReport(
        const std::string& reportName, const std::string& reportingType,
        const bool emitsReadingsUpdate, const bool logToMetricReportsCollection,
        Milliseconds interval, const uint64_t appendLimit,
        const std::string& reportUpdates,
        std::vector<LabeledMetricParameters> metricParams, const bool enabled);
    void loadFromPersistent();

  public:
    static constexpr size_t maxReports{TELEMETRY_MAX_REPORTS};
    static constexpr size_t maxReadingParams{TELEMETRY_MAX_READING_PARAMS};
    static constexpr size_t maxReportNameLength{
        TELEMETRY_MAX_DBUS_PATH_LENGTH -
        std::string_view(Report::reportDir).length()};
    static constexpr Milliseconds minInterval{TELEMETRY_MIN_INTERVAL};
    static constexpr const char* reportManagerIfaceName =
        "xyz.openbmc_project.Telemetry.ReportManager";
    static constexpr const char* reportManagerPath =
        "/xyz/openbmc_project/Telemetry/Reports";
    static constexpr std::array<std::string_view, 2> supportedReportingType = {
        "Periodic", "OnRequest"};
    static constexpr std::array<std::string_view, 3> supportedReportUpdates = {
        "Overwrite", "AppendStopsWhenFull", "AppendWrapsWhenFull"};

    static void verifyReportUpdates(const std::string& reportUpdates);
};
