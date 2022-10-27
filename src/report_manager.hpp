#pragma once

#include "interfaces/json_storage.hpp"
#include "interfaces/report.hpp"
#include "interfaces/report_factory.hpp"
#include "interfaces/report_manager.hpp"
#include "interfaces/trigger_manager.hpp"
#include "report.hpp"
#include "utils/dbus_path_utils.hpp"

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

    ReportManager(const ReportManager&) = delete;
    ReportManager(ReportManager&&) = delete;
    ReportManager& operator=(const ReportManager&) = delete;
    ReportManager& operator=(ReportManager&&) = delete;

    void removeReport(const interfaces::Report* report) override;

  private:
    std::unique_ptr<interfaces::ReportFactory> reportFactory;
    std::unique_ptr<interfaces::JsonStorage> reportStorage;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    std::unique_ptr<sdbusplus::asio::dbus_interface> reportManagerIface;
    std::vector<std::unique_ptr<interfaces::Report>> reports;

    void verifyAddReport(
        const std::string& reportId, const std::string& reportName,
        const ReportingType reportingType, Milliseconds interval,
        const ReportUpdates reportUpdates, const uint64_t appendLimit,
        const std::vector<LabeledMetricParameters>& readingParams);
    interfaces::Report& addReport(
        boost::asio::yield_context& yield, const std::string& reportId,
        const std::string& reportName, const ReportingType reportingType,
        const std::vector<ReportAction>& reportActions, Milliseconds interval,
        const uint64_t appendLimit, const ReportUpdates reportUpdates,
        ReadingParameters metricParams, const bool enabled);
    interfaces::Report&
        addReport(const std::string& reportId, const std::string& reportName,
                  const ReportingType reportingType,
                  const std::vector<ReportAction>& reportActions,
                  Milliseconds interval, const uint64_t appendLimit,
                  const ReportUpdates reportUpdates,
                  std::vector<LabeledMetricParameters> metricParams,
                  const bool enabled, Readings);
    void loadFromPersistent();

  public:
    static constexpr size_t maxReports{TELEMETRY_MAX_REPORTS};
    static constexpr size_t maxNumberMetrics{TELEMETRY_MAX_READING_PARAMS};
    static constexpr Milliseconds minInterval{TELEMETRY_MIN_INTERVAL};
    static constexpr size_t maxAppendLimit{TELEMETRY_MAX_APPEND_LIMIT};
    static constexpr const char* reportManagerIfaceName =
        "xyz.openbmc_project.Telemetry.ReportManager";
    static constexpr const char* reportManagerPath =
        "/xyz/openbmc_project/Telemetry/Reports";
    static constexpr std::string_view reportNameDefault = "Report";

    static_assert(!reportNameDefault.empty(),
                  "Default report name cannot be empty.");
};
