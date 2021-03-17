#include "trigger_factory.hpp"

#include "numeric_threshold.hpp"
#include "sensor.hpp"
#include "trigger.hpp"
#include "trigger_actions.hpp"
#include "utils/dbus_mapper.hpp"
#include "utils/transform.hpp"

TriggerFactory::TriggerFactory(
    std::shared_ptr<sdbusplus::asio::connection> bus,
    std::shared_ptr<sdbusplus::asio::object_server> objServer,
    SensorCache& sensorCache, interfaces::ReportManager& reportManager) :
    bus(std::move(bus)),
    objServer(std::move(objServer)), sensorCache(sensorCache),
    reportManager(reportManager)
{}

std::unique_ptr<interfaces::Trigger> TriggerFactory::make(
    const std::string& name, bool isDiscrete, bool logToJournal,
    bool logToRedfish, bool updateReport,
    const std::vector<std::string>& reportNames,
    interfaces::TriggerManager& triggerManager,
    interfaces::JsonStorage& triggerStorage,
    const LabeledTriggerThresholdParams& labeledThresholdParams,
    const std::vector<LabeledSensorInfo>& labeledSensorsInfo) const
{
    if (isDiscrete)
    {
        throw std::runtime_error("Not implemented!");
    }

    const auto& [sensors, sensorNames] = getSensors(labeledSensorsInfo);

    std::vector<std::shared_ptr<interfaces::Threshold>> thresholds;

    const auto& labeledNumericThresholdParams =
        std::get<std::vector<numeric::LabeledThresholdParam>>(
            labeledThresholdParams);

    for (const auto& labeledThresholdParam : labeledNumericThresholdParams)
    {
        namespace ts = utils::tstring;
        std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;

        numeric::Type type = labeledThresholdParam.at_label<ts::Type>();

        std::chrono::milliseconds dwellTime = std::chrono::milliseconds(
            labeledThresholdParam.at_label<ts::DwellTime>());

        numeric::Direction direction =
            labeledThresholdParam.at_label<ts::Direction>();

        double thresholdValue =
            labeledThresholdParam.at_label<ts::ThresholdValue>();

        if (logToJournal)
        {
            actions.emplace_back(
                std::make_unique<action::LogToJournal>(type, thresholdValue));
        }

        if (logToRedfish)
        {
            actions.emplace_back(
                std::make_unique<action::LogToRedfish>(type, thresholdValue));
        }

        if (updateReport)
        {
            actions.emplace_back(std::make_unique<action::UpdateReport>(
                reportManager, reportNames));
        }

        thresholds.emplace_back(std::make_shared<NumericThreshold>(
            bus->get_io_context(), sensors, sensorNames, std::move(actions),
            dwellTime, direction, thresholdValue));
    }

    return std::make_unique<Trigger>(
        bus->get_io_context(), objServer, name, isDiscrete, logToJournal,
        logToRedfish, updateReport, reportNames, labeledSensorsInfo,
        labeledThresholdParams, std::move(thresholds), triggerManager,
        triggerStorage);
}

std::pair<std::vector<std::shared_ptr<interfaces::Sensor>>,
          std::vector<std::string>>
    TriggerFactory::getSensors(
        const std::vector<LabeledSensorInfo>& labeledSensorsInfo) const
{
    std::vector<std::shared_ptr<interfaces::Sensor>> sensors;
    std::vector<std::string> sensorNames;

    for (const auto& labeledSensorInfo : labeledSensorsInfo)
    {
        namespace ts = utils::tstring;

        const auto& service = labeledSensorInfo.at_label<ts::Service>();
        const auto& sensorPath = labeledSensorInfo.at_label<ts::SensorPath>();
        const auto& metadata = labeledSensorInfo.at_label<ts::SensorMetadata>();

        sensors.emplace_back(sensorCache.makeSensor<Sensor>(
            service, sensorPath, bus->get_io_context(), bus));

        if (metadata.empty())
        {
            sensorNames.emplace_back(sensorPath);
        }
        else
        {
            sensorNames.emplace_back(metadata);
        }
    }

    return {sensors, sensorNames};
}

std::vector<LabeledSensorInfo>
    TriggerFactory::getLabeledSensorsInfo(boost::asio::yield_context& yield,
                                          const SensorsInfo& sensorsInfo) const
{
    auto tree = utils::getSubTreeSensors(yield, bus);

    std::vector<LabeledSensorInfo> labeledSensorsInfo;

    for (const auto& [sensorPath, metadata] : sensorsInfo)
    {
        auto found = std::find_if(
            tree.begin(), tree.end(),
            [&sensorPath](const auto& x) { return x.first == sensorPath; });
        if (found == tree.end())
        {
            throw std::runtime_error("Not found");
        }

        const auto& service = found->second[0].first;
        const auto& path = found->first;

        labeledSensorsInfo.emplace_back(service, path, metadata);
    }

    return labeledSensorsInfo;
}
