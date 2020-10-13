#include "report.hpp"

#include "report_manager.hpp"

Report::Report(boost::asio::io_context& io_context,
               const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
               const std::string& reportName, const std::string& reportingType,
               const bool emitsReadingsSignal,
               const bool logToMetricReportsCollection,
               const std::chrono::milliseconds period,
               const ReadingParameters& metricParams,
               ReportManager& reportManager) :
    name{reportName},
    path{reportPath + name}, interval{period}, objServer(objServer)
{
    reportIntf = objServer->add_interface(path, reportIfaceName);

    reportIntf->register_property(
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
    reportIntf->register_property("Persistency", bool{false});
    reportIntf->register_property("Readings", Readings{});
    reportIntf->register_property("ReportingType", reportingType);
    reportIntf->register_property("ReadingParameters", metricParams);
    reportIntf->register_property("EmitsReadingsUpdate", emitsReadingsSignal);
    reportIntf->register_property("LogToMetricReportsCollection",
                                  logToMetricReportsCollection);

    reportIntf->initialize();

    deleteIface = objServer->add_interface(path, deleteIfaceName);

    deleteIface->register_method("Delete", [this, &io_context, &reportManager] {
        boost::asio::post(io_context, [this, &reportManager] {
            reportManager.removeReport(this);
        });
    });

    deleteIface->initialize();
}

Report::~Report()
{
    objServer->remove_interface(reportIntf);
    objServer->remove_interface(deleteIface);
}
