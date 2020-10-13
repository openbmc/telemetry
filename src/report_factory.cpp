#include "report_factory.hpp"

#include "report.hpp"

ReportFactory::ReportFactory(
    boost::asio::io_context& ioc,
    const std::shared_ptr<sdbusplus::asio::object_server>& objServer) :
    ioc(ioc),
    objServer(objServer)
{}

std::unique_ptr<interfaces::Report> ReportFactory::make(
    const std::string& name, const std::string& reportingType,
    bool emitsReadingsSignal, bool logToMetricReportsCollection,
    std::chrono::milliseconds period, const ReadingParameters& metricParams,
    interfaces::ReportManager& reportManager) const
{
    return std::make_unique<Report>(
        ioc, objServer, name, reportingType, emitsReadingsSignal,
        logToMetricReportsCollection, period, metricParams, reportManager);
}
