#pragma once

#include "interfaces/clock.hpp"
#include "interfaces/json_storage.hpp"
#include "interfaces/metric.hpp"
#include "interfaces/metric_listener.hpp"
#include "interfaces/report.hpp"
#include "interfaces/report_factory.hpp"
#include "interfaces/report_manager.hpp"
#include "state.hpp"
#include "types/error_message.hpp"
#include "types/readings.hpp"
#include "types/report_action.hpp"
#include "types/report_types.hpp"
#include "types/report_updates.hpp"
#include "types/reporting_type.hpp"
#include "utils/circular_vector.hpp"
#include "utils/dbus_path_utils.hpp"
#include "utils/ensure.hpp"
#include "utils/messanger.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <chrono>
#include <memory>
#include <unordered_set>

class Report : public interfaces::Report, public interfaces::MetricListener
{
    class OnChangeContext
    {
      public:
        OnChangeContext(Report& report) : report(report)
        {}

        ~OnChangeContext()
        {
            if (updated)
            {
                report.updateReadings();
            }
        }

        void metricUpdated()
        {
            updated = true;
        }

      private:
        Report& report;
        bool updated = false;
    };

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
           const interfaces::ReportFactory& reportFactory, const bool enabled,
           std::unique_ptr<interfaces::Clock> clock, Readings);
    ~Report();

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
        return path.str;
    }

    void metricUpdated() override;

    void activate();
    void deactivate();

  private:
    std::unique_ptr<sdbusplus::asio::dbus_interface>
        makeReportInterface(const interfaces::ReportFactory& reportFactory);
    static void timerProcForPeriodicReport(boost::system::error_code,
                                           Report& self);
    static void timerProcForOnChangeReport(boost::system::error_code,
                                           Report& self);
    void scheduleTimerForPeriodicReport(Milliseconds interval);
    void scheduleTimerForOnChangeReport();
    uint64_t deduceBufferSize(const ReportUpdates reportUpdatesIn,
                              const ReportingType reportingTypeIn) const;
    void setReadingBuffer(const ReportUpdates newReportUpdates);
    void setReportUpdates(const ReportUpdates newReportUpdates);
    static uint64_t getMetricCount(
        const std::vector<std::shared_ptr<interfaces::Metric>>& metrics);
    interfaces::JsonStorage::FilePath reportFileName() const;
    std::unordered_set<std::string>
        collectTriggerIds(boost::asio::io_context& ioc) const;
    bool storeConfiguration() const;
    bool shouldStoreMetricValues() const;
    void updateReadings();
    void scheduleTimer();
    static std::vector<ErrorMessage> verify(ReportingType, Milliseconds);

    std::string id;
    const sdbusplus::message::object_path path;
    std::string name;
    ReportingType reportingType;
    Milliseconds interval;
    std::unordered_set<ReportAction> reportActions;
    ReadingParameters readingParameters;
    bool persistency = false;
    uint64_t metricCount;
    uint64_t appendLimit;
    ReportUpdates reportUpdates;
    Readings readings = {};
    CircularVector<ReadingData> readingsBuffer;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    std::unique_ptr<sdbusplus::asio::dbus_interface> reportIface;
    std::unique_ptr<sdbusplus::asio::dbus_interface> deleteIface;
    std::vector<std::shared_ptr<interfaces::Metric>> metrics;
    boost::asio::steady_timer timer;
    std::unordered_set<std::string> triggerIds;

    interfaces::JsonStorage& reportStorage;
    std::unique_ptr<interfaces::Clock> clock;
    utils::Messanger messanger;
    std::optional<OnChangeContext> onChangeContext;
    utils::Ensure<std::function<void()>> unregisterFromMetrics;
    State<ReportFlags, Report, ReportFlags::enabled, ReportFlags::valid> state{
        *this};

  public:
    static constexpr const char* reportIfaceName =
        "xyz.openbmc_project.Telemetry.Report";
    static constexpr const char* deleteIfaceName =
        "xyz.openbmc_project.Object.Delete";
    static constexpr size_t reportVersion = 7;
};
