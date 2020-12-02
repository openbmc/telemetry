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
    boost::asio::yield_context& yield, const std::string& name,
    const std::string& reportingType, bool emitsReadingsSignal,
    bool logToMetricReportsCollection, std::chrono::milliseconds period,
    const ReadingParameters& metricParams,
    interfaces::ReportManager& reportManager,
    interfaces::JsonStorage& reportStorage) const
{
    return make(name, reportingType, emitsReadingsSignal,
                logToMetricReportsCollection, period, metricParams,
                reportManager, reportStorage,
                convertMetricParams(yield, metricParams));
}

std::unique_ptr<interfaces::Report> ReportFactory::make(
    const std::string& name, const std::string& reportingType,
    bool emitsReadingsSignal, bool logToMetricReportsCollection,
    std::chrono::milliseconds period, const ReadingParameters& metricParams,
    interfaces::ReportManager& reportManager,
    interfaces::JsonStorage& reportStorage,
    std::vector<LabeledMetricParameters> labeledMetricParams) const
{
    std::vector<std::shared_ptr<interfaces::Metric>> metrics = utils::transform(
        labeledMetricParams,
        [this](const LabeledMetricParameters& param)
            -> std::shared_ptr<interfaces::Metric> {
            return std::make_shared<Metric>(
                getSensors(param.at_index<0>()), param.at_index<1>(),
                param.at_index<2>(), param.at_index<3>());
        });

    return std::make_unique<Report>(
        bus->get_io_context(), objServer, name, reportingType,
        emitsReadingsSignal, logToMetricReportsCollection, period, metricParams,
        reportManager, reportStorage, std::move(metrics));
}

std::vector<std::shared_ptr<interfaces::Sensor>> ReportFactory::getSensors(
    const std::vector<LabeledSensorParameters>& sensorPaths) const
{
    return utils::transform(sensorPaths,
                            [this](const LabeledSensorParameters& param)
                                -> std::shared_ptr<interfaces::Sensor> {
                                using namespace utils::tstring;

                                return sensorCache.makeSensor<Sensor>(
                                    param.at_label<Service>(),
                                    param.at_label<Path>(),
                                    bus->get_io_context(), bus);
                            });
}

std::vector<LabeledMetricParameters> ReportFactory::convertMetricParams(
    boost::asio::yield_context& yield,
    const ReadingParameters& metricParams) const
{
    std::array<const char*, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Value"};
    boost::system::error_code ec;

    auto tree = bus->yield_method_call<std::vector<SensorTree>>(
        yield, ec, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/sensors", 2, interfaces);
    if (ec)
    {
        throw std::runtime_error("Failed to query ObjectMapper!");
    }

    return utils::transform(metricParams, [&tree](const auto& item) {
        std::vector<LabeledSensorParameters> sensors;

        for (const auto& sensorPath : std::get<0>(item))
        {
            auto it = std::find_if(
                tree.begin(), tree.end(),
                [&sensorPath](const auto& v) { return v.first == sensorPath; });

            if (it != tree.end())
            {
                for (const auto& [service, ifaces] : it->second)
                {
                    sensors.emplace_back(service, sensorPath);
                }
            }
        }

        return LabeledMetricParameters(std::move(sensors), std::get<1>(item),
                                       std::get<2>(item), std::get<3>(item));
    });
}
