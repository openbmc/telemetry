#include "report.hpp"

#include "report_manager.hpp"

using Readings = std::tuple<
    uint64_t,
    std::vector<std::tuple<std::string, std::string, double, uint64_t>>>;

Report::Report(boost::asio::io_context& ioc,
               const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
               const std::string& reportName, const std::string& reportingType,
               const bool emitsReadingsSignal,
               const bool logToMetricReportsCollection,
               const std::chrono::milliseconds period,
               const ReadingParameters& metricParams,
               interfaces::ReportManager& reportManager) :
    name{reportName},
    path{reportDir + name}, interval{period}, objServer(objServer)
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
            dbusIface.register_property("Readings", Readings{});
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
}
