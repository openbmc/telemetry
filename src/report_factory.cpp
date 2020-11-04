#include "report_factory.hpp"

#include "metric.hpp"
#include "report.hpp"
#include "sensor.hpp"

ReportFactory::ReportFactory(
    const std::shared_ptr<sdbusplus::asio::connection>& bus,
    const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
    std::unique_ptr<SensorCache> sensorCache) :
    bus(bus),
    objServer(objServer), sensorCache(std::move(sensorCache))
{}

std::unique_ptr<interfaces::Report> ReportFactory::make(
    boost::asio::yield_context& yield, const std::string& name,
    const std::string& reportingType, bool emitsReadingsSignal,
    bool logToMetricReportsCollection, std::chrono::milliseconds period,
    const ReadingParameters& metricParams,
    interfaces::ReportManager& reportManager,
    interfaces::JsonStorage& reportStorage) const
{
    auto sensorTree = getSensorTree(yield);

    std::vector<std::shared_ptr<interfaces::Metric>> metrics;

    for (const auto& params : metricParams)
    {
        std::vector<std::shared_ptr<interfaces::Sensor>> sensors =
            getSensors(sensorTree, std::get<0>(params));

        metrics.emplace_back(
            std::make_shared<Metric>(std::move(sensors), std::get<1>(params),
                                     std::get<2>(params), std::get<3>(params)));
    }

    return std::make_unique<Report>(
        bus->get_io_context(), objServer, name, reportingType,
        emitsReadingsSignal, logToMetricReportsCollection, period, metricParams,
        reportManager, reportStorage, std::move(metrics));
}

std::vector<std::shared_ptr<interfaces::Sensor>> ReportFactory::getSensors(
    const std::vector<ReportFactory::SensorTree>& tree,
    const std::vector<sdbusplus::message::object_path>& sensorPaths) const
{
    std::vector<std::shared_ptr<interfaces::Sensor>> sensors;

    for (const auto& [sensor, ifacesMap] : tree)
    {
        for (const auto& [service, ifaces] : ifacesMap)
        {
            auto it = std::find(sensorPaths.begin(), sensorPaths.end(), sensor);
            if (it != sensorPaths.end())
            {
                sensors.emplace_back(sensorCache->makeSensor<Sensor>(
                    service, sensor, bus->get_io_context(), bus));
            }
        }
    }

    return sensors;
}

std::vector<ReportFactory::SensorTree>
    ReportFactory::getSensorTree(boost::asio::yield_context& yield) const
{
    std::array<const char*, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Value"};
    boost::system::error_code ec;

    auto result = bus->yield_method_call<std::vector<SensorTree>>(
        yield, ec, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/sensors", 2, interfaces);
    if (ec)
    {
        throw std::runtime_error("failed");
    }
    return result;
}
