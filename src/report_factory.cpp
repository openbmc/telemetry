#include "report_factory.hpp"

#include "metric.hpp"
#include "report.hpp"
#include "sensor.hpp"
#include "utils/conversion.hpp"
#include "utils/dbus_mapper.hpp"
#include "utils/transform.hpp"

ReportFactory::ReportFactory(
    std::shared_ptr<sdbusplus::asio::connection> bus,
    const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
    SensorCache& sensorCache) :
    bus(std::move(bus)),
    objServer(objServer), sensorCache(sensorCache)
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
                getSensor(param.at_index<0>()), param.at_index<1>(),
                param.at_index<2>(), param.at_index<3>());
        });

    return std::make_unique<Report>(
        bus->get_io_context(), objServer, name, reportingType,
        emitsReadingsSignal, logToMetricReportsCollection, period, metricParams,
        reportManager, reportStorage, std::move(metrics));
}

std::shared_ptr<interfaces::Sensor>
    ReportFactory::getSensor(const LabeledSensorParameters& sensorPath) const
{
    using namespace utils::tstring;

    return sensorCache.makeSensor<Sensor>(sensorPath.at_label<Service>(),
                                          sensorPath.at_label<Path>(),
                                          bus->get_io_context(), bus);
}

std::vector<LabeledMetricParameters> ReportFactory::convertMetricParams(
    boost::asio::yield_context& yield,
    const ReadingParameters& metricParams) const
{
    auto tree = utils::getSubTreeSensors(yield, bus);

    return utils::transform(metricParams, [&tree](const auto& item) {
        std::vector<LabeledSensorParameters> sensors;

        const auto& [sensorPath, operationType, id, metadata] = item;

        auto it = std::find_if(
            tree.begin(), tree.end(),
            [&sensorPath](const auto& v) { return v.first == sensorPath; });

        if (it != tree.end() && it->second.size() == 1)
        {
            const auto& [service, ifaces] = it->second.front();
            return LabeledMetricParameters(
                LabeledSensorParameters(service, sensorPath),
                utils::stringToOperationType(operationType), id, metadata);
        }

        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::invalid_argument),
            "Invalid number of sensors found");
    });
}
