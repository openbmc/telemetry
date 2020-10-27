#include "report_factory.hpp"

#include "report.hpp"
#include "sensor.hpp"
#include "utils/transform.hpp"

ReportFactory::ReportFactory(
    std::shared_ptr<sdbusplus::asio::connection> bus,
    const std::shared_ptr<sdbusplus::asio::object_server>& objServer) :
    bus(std::move(bus)),
    objServer(objServer)
{}

std::unique_ptr<interfaces::Report> ReportFactory::make(
    std::optional<std::reference_wrapper<boost::asio::yield_context>> yield,
    const std::string& name, const std::string& reportingType,
    bool emitsReadingsSignal, bool logToMetricReportsCollection,
    std::chrono::milliseconds period, const ReadingParameters& metricParams,
    interfaces::ReportManager& reportManager,
    interfaces::JsonStorage& reportStorage) const
{
    std::vector<std::shared_ptr<interfaces::Metric>> metrics;

    return std::make_unique<Report>(
        bus->get_io_context(), objServer, name, reportingType,
        emitsReadingsSignal, logToMetricReportsCollection, period, metricParams,
        reportManager, reportStorage, std::move(metrics));
}
