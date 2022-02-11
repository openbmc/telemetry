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
    const std::string& id, const std::string& name,
    const ReportingType reportingType,
    const std::vector<ReportAction>& reportActions, Milliseconds period,
    uint64_t appendLimit, const ReportUpdates reportUpdates,
    interfaces::ReportManager& reportManager,
    interfaces::JsonStorage& reportStorage,
    std::vector<LabeledMetricParameters> labeledMetricParams,
    bool enabled) const
{
    std::vector<std::shared_ptr<interfaces::Metric>> metrics = utils::transform(
        labeledMetricParams,
        [this](const LabeledMetricParameters& param)
            -> std::shared_ptr<interfaces::Metric> {
            namespace ts = utils::tstring;

            return std::make_shared<Metric>(
                getSensors(param.at_label<ts::SensorPath>()),
                param.at_label<ts::OperationType>(), param.at_label<ts::Id>(),
                param.at_label<ts::CollectionTimeScope>(),
                param.at_label<ts::CollectionDuration>(),
                std::make_unique<Clock>());
        });

    return std::make_unique<Report>(bus->get_io_context(), objServer, id, name,
                                    reportingType, reportActions, period,
                                    appendLimit, reportUpdates, reportManager,
                                    reportStorage, std::move(metrics), *this,
                                    enabled, std::make_unique<Clock>());
}

void ReportFactory::updateMetrics(
    std::vector<std::shared_ptr<interfaces::Metric>>& metrics, bool enabled,
    const ReadingParameters& metricParams) const
{
    auto labeledMetricParams = convertMetricParams(metricParams);
    std::vector<std::shared_ptr<interfaces::Metric>> oldMetrics = metrics;
    std::vector<std::shared_ptr<interfaces::Metric>> newMetrics;

    for (const auto& labeledMetricParam : labeledMetricParams)
    {
        auto existing = std::find_if(oldMetrics.begin(), oldMetrics.end(),
                                     [labeledMetricParam](auto metric) {
                                         return labeledMetricParam ==
                                                metric->dumpConfiguration();
                                     });

        if (existing != oldMetrics.end())
        {
            newMetrics.emplace_back(*existing);
            oldMetrics.erase(existing);
            continue;
        }

        namespace ts = utils::tstring;
        newMetrics.emplace_back(std::make_shared<Metric>(
            getSensors(labeledMetricParam.at_label<ts::SensorPath>()),
            labeledMetricParam.at_label<ts::OperationType>(),
            labeledMetricParam.at_label<ts::Id>(),
            labeledMetricParam.at_label<ts::CollectionTimeScope>(),
            labeledMetricParam.at_label<ts::CollectionDuration>(),
            std::make_unique<Clock>()));

        if (enabled)
        {
            newMetrics.back()->initialize();
        }
    }

    if (enabled)
    {
        for (auto& metric : oldMetrics)
        {
            metric->deinitialize();
        }
    }

    metrics = std::move(newMetrics);
}

Sensors ReportFactory::getSensors(
    const std::vector<LabeledSensorInfo>& sensorPaths) const
{
    using namespace utils::tstring;

    return utils::transform(
        sensorPaths,
        [this](const LabeledSensorInfo& sensorPath)
            -> std::shared_ptr<interfaces::Sensor> {
            return sensorCache.makeSensor<Sensor>(
                sensorPath.at_label<Service>(), sensorPath.at_label<Path>(),
                sensorPath.at_label<Metadata>(), bus->get_io_context(), bus);
        });
}

std::vector<LabeledMetricParameters> ReportFactory::convertMetricParams(
    boost::asio::yield_context& yield,
    const ReadingParameters& metricParams) const
{
    if (metricParams.empty())
    {
        return {};
    }
    auto tree = utils::getSubTreeSensors(yield, bus);
    return getMetricParamsFromSensorTree(metricParams, tree);
}

std::vector<LabeledMetricParameters> ReportFactory::convertMetricParams(
    const ReadingParameters& metricParams) const
{
    if (metricParams.empty())
    {
        return {};
    }
    auto tree = utils::getSubTreeSensors(bus);
    return getMetricParamsFromSensorTree(metricParams, tree);
}

std::vector<LabeledMetricParameters>
    ReportFactory::getMetricParamsFromSensorTree(
        const ReadingParameters& metricParams,
        const std::vector<utils::SensorTree>& tree) const
{
    return utils::transform(metricParams, [&tree](const auto& item) {
        auto [sensorPaths, operationType, id, collectionTimeScope,
              collectionDuration] = item;

        std::vector<LabeledSensorInfo> sensorParameters;

        for (const auto& [sensorPath, metadata] : sensorPaths)
        {
            auto it = std::find_if(
                tree.begin(), tree.end(),
                [path = sensorPath](const auto& v) { return v.first == path; });

            if (it != tree.end() && it->second.size() == 1)
            {
                const auto& [service, ifaces] = it->second.front();
                sensorParameters.emplace_back(service, sensorPath, metadata);
            }
        }

        if (sensorParameters.size() != sensorPaths.size())
        {
            throw sdbusplus::exception::SdBusError(
                static_cast<int>(std::errc::invalid_argument),
                "Could not find service for provided sensors");
        }

        if (operationType.empty())
        {
            operationType = utils::enumToString(OperationType::avg);
        }
        else if (operationType == "SINGLE")
        {
            operationType = utils::enumToString(OperationType::avg);
            collectionTimeScope =
                utils::enumToString(CollectionTimeScope::point);
        }

        if (collectionTimeScope.empty())
        {
            collectionTimeScope =
                utils::enumToString(CollectionTimeScope::point);
        }

        return LabeledMetricParameters(
            std::move(sensorParameters), utils::toOperationType(operationType),
            id, utils::toCollectionTimeScope(collectionTimeScope),
            CollectionDuration(Milliseconds(collectionDuration)));
    });
}
