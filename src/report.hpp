#pragma once

#include "interfaces/clock.hpp"
#include "interfaces/json_storage.hpp"
#include "interfaces/metric.hpp"
#include "interfaces/report.hpp"
#include "interfaces/report_manager.hpp"
#include "types/report_action.hpp"
#include "types/report_types.hpp"
#include "types/report_updates.hpp"
#include "types/reporting_type.hpp"
#include "utils/circular_vector.hpp"
#include "utils/messanger.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <chrono>
#include <memory>
#include <unordered_set>

class Report : public interfaces::Report
{
  public:
    Report(boost::asio::io_context& ioc,
           const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
           const std::string& reportId, const std::string& reportName,
           const ReportingType reportingType,
           std::vector<ReportAction> reportActions, const Milliseconds period,
           const uint64_t appendLimitIn, const ReportUpdates reportUpdatesIn,
           interfaces::ReportManager& reportManager,
           interfaces::JsonStorage& reportStorage,
           std::vector<std::shared_ptr<interfaces::Metric>> metrics,
           const bool enabled, std::unique_ptr<interfaces::Clock> clock);

    Report(const Report&) = delete;
    Report(Report&&) = delete;
    Report& operator=(const Report&) = delete;
    Report& operator=(Report&&) = delete;

    std::string getId() const override
    {
        return id;
    }

    std::string getPath() const override
    {
        return reportDir + id;
    }

  private:
    std::unique_ptr<sdbusplus::asio::dbus_interface> makeReportInterface();
    static void timerProc(boost::system::error_code, Report& self);
    void scheduleTimer(Milliseconds interval);
    std::optional<uint64_t>
        deduceAppendLimit(const uint64_t appendLimitIn) const;
    uint64_t deduceBufferSize(const ReportUpdates reportUpdatesIn,
                              const ReportingType reportingTypeIn) const;
    void setReportUpdates(const ReportUpdates newReportUpdates);
    static uint64_t getSensorCount(
        std::vector<std::shared_ptr<interfaces::Metric>>& metrics);
    interfaces::JsonStorage::FilePath fileName() const;
    std::unordered_set<std::string>
        collectTriggerIds(boost::asio::io_context& ioc) const;
    bool storeConfiguration() const;
    void updateReadings();

    std::string id;
    std::string name;
    ReportingType reportingType;
    Milliseconds interval;
    std::vector<ReportAction> reportActions;
    ReadingParametersPastVersion readingParametersPastVersion;
    ReadingParameters readingParameters;
    bool persistency = false;
    uint64_t sensorCount;
    std::optional<uint64_t> appendLimit;
    ReportUpdates reportUpdates;
    CircularVector<ReadingData> readingsBuffer;
    Readings readings = {};
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    std::unique_ptr<sdbusplus::asio::dbus_interface> reportIface;
    std::unique_ptr<sdbusplus::asio::dbus_interface> deleteIface;
    std::vector<std::shared_ptr<interfaces::Metric>> metrics;
    boost::asio::steady_timer timer;
    std::unordered_set<std::string> triggerIds;

    interfaces::JsonStorage& reportStorage;
    bool enabled;
    std::unique_ptr<interfaces::Clock> clock;
    utils::Messanger messanger;

  public:
    static constexpr const char* reportIfaceName =
        "xyz.openbmc_project.Telemetry.Report";
    static constexpr const char* reportDir =
        "/xyz/openbmc_project/Telemetry/Reports/";
    static constexpr const char* deleteIfaceName =
        "xyz.openbmc_project.Object.Delete";
    static constexpr size_t reportVersion = 6;
};
