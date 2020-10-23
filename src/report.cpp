#include "report.hpp"

#include "report_manager.hpp"

Report::Report(boost::asio::io_context& ioc,
               const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
               const std::string& reportName, const std::string& reportingType,
               const bool emitsReadingsSignal,
               const bool logToMetricReportsCollection,
               const std::chrono::milliseconds period,
               const ReadingParameters& metricParams,
               interfaces::ReportManager& reportManager,
               std::vector<std::shared_ptr<interfaces::Metric>> metrics) :
    name(reportName),
    path(reportDir + name), interval(period), objServer(objServer),
    metrics(std::move(metrics)), timer(ioc)
{
    reportIface = objServer->add_unique_interface(
        path, reportIfaceName,
        [this, &reportingType, &emitsReadingsSignal,
         &logToMetricReportsCollection, &metricParams](auto& dbusIface) {
            dbusIface.register_property(
                "Interval", static_cast<uint64_t>(interval.count()),
                [this](const uint64_t newVal, uint64_t& actualVal) {
                    std::chrono::milliseconds newValT(newVal);
                    if (newValT < ReportManager::minInterval)
                    {
                        return false;
                    }
                    actualVal = newVal;
                    interval = newValT;
                    return true;
                });
            dbusIface.register_property("Persistency", bool{false});
            dbusIface.register_property_r(
                "Readings", readings,
                sdbusplus::vtable::property_::emits_change,
                [this](const auto&) { return readings; });
            dbusIface.register_property("ReportingType", reportingType);
            dbusIface.register_property("ReadingParameters", metricParams);
            dbusIface.register_property("EmitsReadingsUpdate",
                                        emitsReadingsSignal);
            dbusIface.register_property("LogToMetricReportsCollection",
                                        logToMetricReportsCollection);
        });

    deleteIface = objServer->add_unique_interface(
        path, deleteIfaceName, [this, &ioc, &reportManager](auto& dbusIface) {
            dbusIface.register_method("Delete", [this, &ioc, &reportManager] {
                boost::asio::post(ioc, [this, &reportManager] {
                    reportManager.removeReport(this);
                });
            });
        });

    if (reportingType == "Periodic")
    {
        scheduleTimer(interval);
    }
}

void Report::timerProc(boost::system::error_code ec, Report& self)
{
    if (ec)
    {
        return;
    }

    self.updateReadings();
    self.scheduleTimer(self.interval);
}

void Report::scheduleTimer(std::chrono::milliseconds timerInterval)
{
    timer.expires_from_now(
        boost::posix_time::milliseconds(timerInterval.count()));
    timer.async_wait(
        [this](boost::system::error_code ec) { timerProc(ec, *this); });
}

void Report::updateReadings()
{
    std::get<0>(readingsCache) = std::time(0);
    readings = readingsCache;
    reportIface->signal_property("Readings");
}
