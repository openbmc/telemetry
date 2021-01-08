#pragma once

#include "interfaces/json_storage.hpp"
#include "interfaces/metric.hpp"
#include "interfaces/report.hpp"
#include "interfaces/report_manager.hpp"
#include "interfaces/types.hpp"

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
           const std::string& reportName, const std::string& reportingType,
           const bool emitsReadingsSignal,
           const bool logToMetricReportsCollection,
           const std::chrono::milliseconds period,
           const ReadingParameters& metricParams,
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
    static void timerProc(boost::system::error_code, Report& self);
    void scheduleTimer(std::chrono::milliseconds interval);

    const std::string name;
    const std::string path;
    std::string reportingType;
    std::chrono::milliseconds interval;
    bool emitsReadingsUpdate;
    bool logToMetricReportsCollection;
    ReadingParameters readingParameters;
    bool persistency;
    Readings readings = {};
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
    static constexpr size_t reportVersion = 2;
};
