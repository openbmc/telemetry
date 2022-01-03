#include "trigger_factory.hpp"

#include "discrete_threshold.hpp"
#include "numeric_threshold.hpp"
#include "on_change_threshold.hpp"
#include "sensor.hpp"
#include "trigger.hpp"
#include "trigger_actions.hpp"
#include "utils/dbus_mapper.hpp"
#include "utils/transform.hpp"

namespace ts = utils::tstring;

TriggerFactory::TriggerFactory(
    std::shared_ptr<sdbusplus::asio::connection> bus,
    std::shared_ptr<sdbusplus::asio::object_server> objServer,
    SensorCache& sensorCache, interfaces::ReportManager& reportManager) :
    bus(std::move(bus)),
    objServer(std::move(objServer)), sensorCache(sensorCache),
    reportManager(reportManager)
{}

std::unique_ptr<interfaces::Trigger> TriggerFactory::make(
    const std::string& id, const std::string& name,
    const std::vector<std::string>& triggerActionsIn,
    const std::vector<std::string>& reportIds,
    interfaces::TriggerManager& triggerManager,
    interfaces::JsonStorage& triggerStorage,
    const LabeledTriggerThresholdParams& labeledThresholdParams,
    const std::vector<LabeledSensorInfo>& labeledSensorsInfo) const
{
    const auto& [sensors, sensorNames] = getSensors(labeledSensorsInfo);
    std::vector<TriggerAction> triggerActions;
    std::transform(triggerActionsIn.begin(), triggerActionsIn.end(),
                   std::back_inserter(triggerActions),
                   [](const auto& triggerActionStr) {
                       return toTriggerAction(triggerActionStr);
                   });
    std::vector<std::shared_ptr<interfaces::Threshold>> thresholds;

    if (isTriggerThresholdDiscrete(labeledThresholdParams))
    {
        const auto& labeledDiscreteThresholdParams =
            std::get<std::vector<discrete::LabeledThresholdParam>>(
                labeledThresholdParams);
        for (const auto& labeledThresholdParam : labeledDiscreteThresholdParams)
        {
            std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;

            std::string thresholdName =
                labeledThresholdParam.at_label<ts::UserId>();
            discrete::Severity severity =
                labeledThresholdParam.at_label<ts::Severity>();
            auto dwellTime =
                Milliseconds(labeledThresholdParam.at_label<ts::DwellTime>());
            std::string thresholdValue =
                labeledThresholdParam.at_label<ts::ThresholdValue>();

            action::discrete::fillActions(actions, triggerActions, severity,
                                          reportManager, reportIds);

            thresholds.emplace_back(std::make_shared<DiscreteThreshold>(
                bus->get_io_context(), sensors, sensorNames, std::move(actions),
                Milliseconds(dwellTime), std::stod(thresholdValue),
                thresholdName));
        }
        if (labeledDiscreteThresholdParams.empty())
        {
            std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;
            action::discrete::onChange::fillActions(actions, triggerActions,
                                                    reportManager, reportIds);

            thresholds.emplace_back(std::make_shared<OnChangeThreshold>(
                sensors, sensorNames, std::move(actions)));
        }
    }
    else
    {
        const auto& labeledNumericThresholdParams =
            std::get<std::vector<numeric::LabeledThresholdParam>>(
                labeledThresholdParams);

        for (const auto& labeledThresholdParam : labeledNumericThresholdParams)
        {
            std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;
            auto type = labeledThresholdParam.at_label<ts::Type>();
            auto dwellTime =
                Milliseconds(labeledThresholdParam.at_label<ts::DwellTime>());
            auto direction = labeledThresholdParam.at_label<ts::Direction>();
            auto thresholdValue =
                double{labeledThresholdParam.at_label<ts::ThresholdValue>()};

            action::numeric::fillActions(actions, triggerActions, type,
                                         thresholdValue, reportManager,
                                         reportIds);

            thresholds.emplace_back(std::make_shared<NumericThreshold>(
                bus->get_io_context(), sensors, sensorNames, std::move(actions),
                dwellTime, direction, thresholdValue));
        }
    }

    return std::make_unique<Trigger>(
        bus->get_io_context(), objServer, id, name, triggerActionsIn, reportIds,
        labeledSensorsInfo, labeledThresholdParams, std::move(thresholds),
        triggerManager, triggerStorage);
}

std::pair<Sensors, std::vector<std::string>> TriggerFactory::getSensors(
    const std::vector<LabeledSensorInfo>& labeledSensorsInfo) const
{
    Sensors sensors;
    std::vector<std::string> sensorNames;

    for (const auto& labeledSensorInfo : labeledSensorsInfo)
    {
        const auto& service = labeledSensorInfo.at_label<ts::Service>();
        const auto& sensorPath = labeledSensorInfo.at_label<ts::SensorPath>();
        const auto& metadata = labeledSensorInfo.at_label<ts::Metadata>();

        sensors.emplace_back(sensorCache.makeSensor<Sensor>(
            service, sensorPath, metadata, bus->get_io_context(), bus));

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

    return utils::transform(sensorsInfo, [&tree](const auto& item) {
        const auto& [sensorPath, metadata] = item;
        auto found = std::find_if(tree.begin(), tree.end(),
                                  [sensorPath = sensorPath](const auto& x) {
                                      return x.first == sensorPath;
                                  });

        if (tree.end() != found)
        {
            const auto& [service, ifaces] = found->second.front();
            return LabeledSensorInfo(service, sensorPath, metadata);
        }
        throw std::runtime_error("Not found");
    });
}
