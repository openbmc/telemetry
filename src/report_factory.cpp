#include "report_factory.hpp"

#include "metric.hpp"
#include "report.hpp"
#include "sensor.hpp"
#include "utils/clock.hpp"
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
    const std::string& name, const std::string& reportingType,
    bool emitsReadingsSignal, bool logToMetricReportsCollection,
    DurationType period, interfaces::ReportManager& reportManager,
    interfaces::JsonStorage& reportStorage,
    std::vector<LabeledMetricParameters> labeledMetricParams) const
{
    std::vector<std::shared_ptr<interfaces::Metric>> metrics = utils::transform(
        labeledMetricParams,
        [this](const LabeledMetricParameters& param)
            -> std::shared_ptr<interfaces::Metric> {
            return std::make_shared<Metric>(
                getSensors(param.at_index<0>()), param.at_index<1>(),
                param.at_index<2>(), param.at_index<3>(), param.at_index<4>(),
                param.at_index<5>(), std::make_unique<Clock>());
        });

    return std::make_unique<Report>(
        bus->get_io_context(), objServer, name, reportingType,
        emitsReadingsSignal, logToMetricReportsCollection, period,
        reportManager, reportStorage, std::move(metrics));
}

std::vector<std::shared_ptr<interfaces::Sensor>> ReportFactory::getSensors(
    const std::vector<LabeledSensorParameters>& sensorPaths) const
{
    using namespace utils::tstring;

    return utils::transform(sensorPaths,
                            [this](const LabeledSensorParameters& sensorPath)
                                -> std::shared_ptr<interfaces::Sensor> {
                                return sensorCache.makeSensor<Sensor>(
                                    sensorPath.at_label<Service>(),
                                    sensorPath.at_label<Path>(),
                                    bus->get_io_context(), bus);
                            });
}

std::vector<LabeledMetricParameters> ReportFactory::convertMetricParams(
    boost::asio::yield_context& yield,
    const ReadingParameters& metricParams) const
{
    auto tree = utils::getSubTreeSensors(yield, bus);

    return utils::transform(metricParams, [&tree](const auto& item) {
        const auto& [sensorPaths, operationType, id, metadata,
                     collectionTimeScope, collectionDuration] = item;

        std::vector<LabeledSensorParameters> sensorParameters;

        for (const auto& sensorPath : sensorPaths)
        {
            auto it = std::find_if(
                tree.begin(), tree.end(),
                [&sensorPath](const auto& v) { return v.first == sensorPath; });

            if (it != tree.end() && it->second.size() == 1)
            {
                const auto& [service, ifaces] = it->second.front();
                sensorParameters.emplace_back(service, sensorPath);
            }
        }

        if (sensorParameters.size() != sensorPaths.size())
        {
            throw sdbusplus::exception::SdBusError(
                static_cast<int>(std::errc::invalid_argument),
                "Could not find service for provided sensors");
        }

        return LabeledMetricParameters(
            std::move(sensorParameters),
            utils::stringToOperationType(operationType), id, metadata,
            utils::stringToCollectionTimeScope(collectionTimeScope),
            CollectionDuration(DurationType(collectionDuration)));
    });
}
