#pragma once

#include "interfaces/json_storage.hpp"
#include "interfaces/metric.hpp"
#include "interfaces/report.hpp"
#include "interfaces/report_manager.hpp"
#include "types/report_types.hpp"
#include "utils/circular_vector.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <chrono>
#include <memory>

class Report : public interfaces::Report
{
  public:
    Report(boost::asio::io_context& ioc,
           const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
           const std::string& reportName, const ReportingType reportingType,
           const bool emitsReadingsSignal,
           const bool logToMetricReportsCollection, const Milliseconds period,
           uint64_t appendLimitIn, const ReportUpdates reportUpdatesIn,
           interfaces::ReportManager& reportManager,
           interfaces::JsonStorage& reportStorage,
           std::vector<std::shared_ptr<interfaces::Metric>> metrics);
    ~Report() = default;

    Report(const Report&) = delete;
    Report(Report&&) = delete;
    Report& operator=(const Report&) = delete;
    Report& operator=(Report&&) = delete;

    std::string getName() const override
    {
        return name;
    }

    std::string getPath() const override
    {
        return path;
    }

    void updateReadings() override;
    bool storeConfiguration() const;

  private:
    std::unique_ptr<sdbusplus::asio::dbus_interface> makeReportInterface();
    static void timerProc(boost::system::error_code, Report& self);
    void scheduleTimer(Milliseconds interval);
    uint64_t deduceAppendLimit(
        const uint64_t appendLimitIn, const ReportUpdates reportUpdatesIn,
        const ReportingType reportingTypeIn,
        const std::vector<std::shared_ptr<interfaces::Metric>>& metricsIn)
        const;
    ReportUpdates
        deduceReportUpdates(const ReportUpdates reportUpdatesIn,
                            const ReportingType reportingTypeIn) const;

    const std::string name;
    const std::string path;
    ReportingType reportingType;
    Milliseconds interval;
    bool emitsReadingsUpdate;
    bool logToMetricReportsCollection;
    ReadingParametersPastVersion readingParametersPastVersion;
    ReadingParameters readingParameters;
    bool persistency = false;
    uint64_t appendLimit;
    CircularVector<ReadingData> readingsBuffer;
    Readings readings = {};
    ReportUpdates reportUpdates;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    std::unique_ptr<sdbusplus::asio::dbus_interface> reportIface;
    std::unique_ptr<sdbusplus::asio::dbus_interface> deleteIface;
    std::vector<std::shared_ptr<interfaces::Metric>> metrics;
    boost::asio::steady_timer timer;

    interfaces::JsonStorage::FilePath fileName;
    interfaces::JsonStorage& reportStorage;

  public:
    static constexpr const char* reportIfaceName =
        "xyz.openbmc_project.Telemetry.Report";
    static constexpr const char* reportDir =
        "/xyz/openbmc_project/Telemetry/Reports/";
    static constexpr const char* deleteIfaceName =
        "xyz.openbmc_project.Object.Delete";
    static constexpr size_t reportVersion = 4;
};
