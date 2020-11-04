#include "report_factory.hpp"

#include "metric.hpp"
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
    std::optional<std::vector<ReportFactory::SensorTree>> sensorTree;

    std::vector<std::shared_ptr<interfaces::Metric>> metrics;
    metrics.reserve(metricParams.size());

    for (const auto& [sensorPaths, op, id, metadata] : metricParams)
    {
        if (!sensorTree && yield && sensorPaths.size() > 0)
        {
            sensorTree = getSensorTree(*yield);
        }

        std::vector<std::shared_ptr<interfaces::Sensor>> sensors =
            getSensors(sensorTree, sensorPaths);

        metrics.emplace_back(
            std::make_shared<Metric>(std::move(sensors), op, id, metadata));
    }

    return std::make_unique<Report>(
        bus->get_io_context(), objServer, name, reportingType,
        emitsReadingsSignal, logToMetricReportsCollection, period, metricParams,
        reportManager, reportStorage, std::move(metrics));
}

std::vector<std::shared_ptr<interfaces::Sensor>> ReportFactory::getSensors(
    const std::optional<std::vector<ReportFactory::SensorTree>>& tree,
    const std::vector<sdbusplus::message::object_path>& sensorPaths) const
{
    if (tree)
    {
        std::vector<std::shared_ptr<interfaces::Sensor>> sensors;

        for (const auto& [sensor, ifacesMap] : *tree)
        {
            auto it = std::find(sensorPaths.begin(), sensorPaths.end(), sensor);
            if (it != sensorPaths.end())
            {
                for (const auto& [service, ifaces] : ifacesMap)
                {
                    sensors.emplace_back(sensorCache->makeSensor<Sensor>(
                        service, sensor, bus->get_io_context(), bus));
                }
            }
        }

        return sensors;
    }
    else
    {
        return utils::transform(
            sensorPaths,
            [this](const std::string& sensor)
                -> std::shared_ptr<interfaces::Sensor> {
                std::string::size_type pos = sensor.find_first_of(":");
                auto service = sensor.substr(0, pos);
                auto path = sensor.substr(pos + 1);
                return sensorCache->makeSensor<Sensor>(
                    service, path, bus->get_io_context(), bus);
            });
    }
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
