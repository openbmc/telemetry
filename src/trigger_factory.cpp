#include "trigger_factory.hpp"

#include "discrete_threshold.hpp"
#include "numeric_threshold.hpp"
#include "on_change_threshold.hpp"
#include "sensor.hpp"
#include "trigger.hpp"
#include "trigger_actions.hpp"
#include "utils/clock.hpp"
#include "utils/dbus_mapper.hpp"
#include "utils/transform.hpp"

namespace ts = utils::tstring;

TriggerFactory::TriggerFactory(
    std::shared_ptr<sdbusplus::asio::connection> bus,
    std::shared_ptr<sdbusplus::asio::object_server> objServer,
    SensorCache& sensorCache) :
    bus(std::move(bus)),
    objServer(std::move(objServer)), sensorCache(sensorCache)
{}

void TriggerFactory::updateDiscreteThresholds(
    std::vector<std::shared_ptr<interfaces::Threshold>>& currentThresholds,
    const std::string& triggerId,
    const std::vector<TriggerAction>& triggerActions,
    const std::shared_ptr<std::vector<std::string>>& reportIds,
    const Sensors& sensors,
    const std::vector<discrete::LabeledThresholdParam>& newParams) const
{
    auto oldThresholds = currentThresholds;
    std::vector<std::shared_ptr<interfaces::Threshold>> newThresholds;

    bool isCurrentOnChange = false;
    if (oldThresholds.size() == 1 &&
        std::holds_alternative<std::monostate>(
            oldThresholds.back()->getThresholdParam()))
    {
        isCurrentOnChange = true;
    }

    newThresholds.reserve(newParams.size());

    if (!isCurrentOnChange)
    {
        for (const auto& labeledThresholdParam : newParams)
        {
            auto paramChecker = [labeledThresholdParam](auto threshold) {
                return labeledThresholdParam ==
                       std::get<discrete::LabeledThresholdParam>(
                           threshold->getThresholdParam());
            };
            if (auto existing = std::find_if(oldThresholds.begin(),
                                             oldThresholds.end(), paramChecker);
                existing != oldThresholds.end())
            {
                newThresholds.emplace_back(*existing);
                oldThresholds.erase(existing);
                continue;
            }

            makeDiscreteThreshold(newThresholds, triggerId, triggerActions,
                                  reportIds, sensors, labeledThresholdParam);
        }
    }
    else
    {
        for (const auto& labeledThresholdParam : newParams)
        {
            makeDiscreteThreshold(newThresholds, triggerId, triggerActions,
                                  reportIds, sensors, labeledThresholdParam);
        }
    }
    if (newParams.empty())
    {
        if (isCurrentOnChange)
        {
            newThresholds.emplace_back(*oldThresholds.begin());
        }
        else
        {
            makeOnChangeThreshold(newThresholds, triggerId, triggerActions,
                                  reportIds, sensors);
        }
    }
    currentThresholds = std::move(newThresholds);
}

void TriggerFactory::updateNumericThresholds(
    std::vector<std::shared_ptr<interfaces::Threshold>>& currentThresholds,
    const std::string& triggerId,
    const std::vector<TriggerAction>& triggerActions,
    const std::shared_ptr<std::vector<std::string>>& reportIds,
    const Sensors& sensors,
    const std::vector<numeric::LabeledThresholdParam>& newParams) const
{
    auto oldThresholds = currentThresholds;
    std::vector<std::shared_ptr<interfaces::Threshold>> newThresholds;

    newThresholds.reserve(newParams.size());

    for (const auto& labeledThresholdParam : newParams)
    {
        auto paramChecker = [labeledThresholdParam](auto threshold) {
            return labeledThresholdParam ==
                   std::get<numeric::LabeledThresholdParam>(
                       threshold->getThresholdParam());
        };
        if (auto existing = std::find_if(oldThresholds.begin(),
                                         oldThresholds.end(), paramChecker);
            existing != oldThresholds.end())
        {
            newThresholds.emplace_back(*existing);
            oldThresholds.erase(existing);
            continue;
        }

        makeNumericThreshold(newThresholds, triggerId, triggerActions,
                             reportIds, sensors, labeledThresholdParam);
    }
    currentThresholds = std::move(newThresholds);
}

void TriggerFactory::updateThresholds(
    std::vector<std::shared_ptr<interfaces::Threshold>>& currentThresholds,
    const std::string& triggerId,
    const std::vector<TriggerAction>& triggerActions,
    const std::shared_ptr<std::vector<std::string>>& reportIds,
    const Sensors& sensors,
    const LabeledTriggerThresholdParams& newParams) const
{
    if (isTriggerThresholdDiscrete(newParams))
    {
        const auto& labeledDiscreteThresholdParams =
            std::get<std::vector<discrete::LabeledThresholdParam>>(newParams);

        updateDiscreteThresholds(currentThresholds, triggerId, triggerActions,
                                 reportIds, sensors,
                                 labeledDiscreteThresholdParams);
    }
    else
    {
        const auto& labeledNumericThresholdParams =
            std::get<std::vector<numeric::LabeledThresholdParam>>(newParams);

        updateNumericThresholds(currentThresholds, triggerId, triggerActions,
                                reportIds, sensors,
                                labeledNumericThresholdParams);
    }
}

void TriggerFactory::makeDiscreteThreshold(
    std::vector<std::shared_ptr<interfaces::Threshold>>& thresholds,
    const std::string& triggerId,
    const std::vector<TriggerAction>& triggerActions,
    const std::shared_ptr<std::vector<std::string>>& reportIds,
    const Sensors& sensors,
    const discrete::LabeledThresholdParam& thresholdParam) const
{
    std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;

    const std::string& thresholdName = thresholdParam.at_label<ts::UserId>();
    discrete::Severity severity = thresholdParam.at_label<ts::Severity>();
    auto dwellTime = Milliseconds(thresholdParam.at_label<ts::DwellTime>());
    const std::string& thresholdValue =
        thresholdParam.at_label<ts::ThresholdValue>();

    action::discrete::fillActions(actions, triggerActions, severity,
                                  bus->get_io_context(), reportIds);

    thresholds.emplace_back(std::make_shared<DiscreteThreshold>(
        bus->get_io_context(), triggerId, sensors, std::move(actions),
        Milliseconds(dwellTime), thresholdValue, thresholdName, severity,
        std::make_unique<Clock>()));
}

void TriggerFactory::makeNumericThreshold(
    std::vector<std::shared_ptr<interfaces::Threshold>>& thresholds,
    const std::string& triggerId,
    const std::vector<TriggerAction>& triggerActions,
    const std::shared_ptr<std::vector<std::string>>& reportIds,
    const Sensors& sensors,
    const numeric::LabeledThresholdParam& thresholdParam) const
{
    std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;

    auto type = thresholdParam.at_label<ts::Type>();
    auto dwellTime = Milliseconds(thresholdParam.at_label<ts::DwellTime>());
    auto direction = thresholdParam.at_label<ts::Direction>();
    auto thresholdValue = double{thresholdParam.at_label<ts::ThresholdValue>()};

    action::numeric::fillActions(actions, triggerActions, type, thresholdValue,
                                 bus->get_io_context(), reportIds);

    thresholds.emplace_back(std::make_shared<NumericThreshold>(
        bus->get_io_context(), triggerId, sensors, std::move(actions),
        dwellTime, direction, thresholdValue, type, std::make_unique<Clock>()));
}

void TriggerFactory::makeOnChangeThreshold(
    std::vector<std::shared_ptr<interfaces::Threshold>>& thresholds,
    const std::string& triggerId,
    const std::vector<TriggerAction>& triggerActions,
    const std::shared_ptr<std::vector<std::string>>& reportIds,
    const Sensors& sensors) const
{
    std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;

    action::discrete::onChange::fillActions(actions, triggerActions,
                                            bus->get_io_context(), reportIds);

    thresholds.emplace_back(std::make_shared<OnChangeThreshold>(
        triggerId, sensors, std::move(actions), std::make_unique<Clock>()));
}

std::unique_ptr<interfaces::Trigger> TriggerFactory::make(
    const std::string& idIn, const std::string& name,
    const std::vector<std::string>& triggerActionsIn,
    const std::vector<std::string>& reportIdsIn,
    interfaces::TriggerManager& triggerManager,
    interfaces::JsonStorage& triggerStorage,
    const LabeledTriggerThresholdParams& labeledThresholdParams,
    const std::vector<LabeledSensorInfo>& labeledSensorsInfo) const
{
    const auto& sensors = getSensors(labeledSensorsInfo);
    auto triggerActions = utils::transform(triggerActionsIn,
                                           [](const auto& triggerActionStr) {
        return toTriggerAction(triggerActionStr);
    });
    std::vector<std::shared_ptr<interfaces::Threshold>> thresholds;
    auto id = std::make_unique<const std::string>(idIn);

    auto reportIds = std::make_shared<std::vector<std::string>>(reportIdsIn);

    updateThresholds(thresholds, *id, triggerActions, reportIds, sensors,
                     labeledThresholdParams);

    return std::make_unique<Trigger>(
        bus->get_io_context(), objServer, std::move(id), name, triggerActions,
        reportIds, std::move(thresholds), triggerManager, triggerStorage, *this,
        sensors);
}

Sensors TriggerFactory::getSensors(
    const std::vector<LabeledSensorInfo>& labeledSensorsInfo) const
{
    Sensors sensors;
    updateSensors(sensors, labeledSensorsInfo);
    return sensors;
}

void TriggerFactory::updateSensors(
    Sensors& currentSensors,
    const std::vector<LabeledSensorInfo>& labeledSensorsInfo) const
{
    Sensors oldSensors = currentSensors;
    Sensors newSensors;

    for (const auto& labeledSensorInfo : labeledSensorsInfo)
    {
        auto existing = std::find_if(oldSensors.begin(), oldSensors.end(),
                                     [labeledSensorInfo](auto sensor) {
            return labeledSensorInfo == sensor->getLabeledSensorInfo();
        });

        if (existing != oldSensors.end())
        {
            newSensors.emplace_back(*existing);
            oldSensors.erase(existing);
            continue;
        }

        const auto& service = labeledSensorInfo.at_label<ts::Service>();
        const auto& sensorPath = labeledSensorInfo.at_label<ts::Path>();
        const auto& metadata = labeledSensorInfo.at_label<ts::Metadata>();

        newSensors.emplace_back(sensorCache.makeSensor<Sensor>(
            service, sensorPath, metadata, bus->get_io_context(), bus));
    }

    currentSensors = std::move(newSensors);
}

std::vector<LabeledSensorInfo>
    TriggerFactory::getLabeledSensorsInfo(boost::asio::yield_context& yield,
                                          const SensorsInfo& sensorsInfo) const
{
    if (sensorsInfo.empty())
    {
        return {};
    }
    auto tree = utils::getSubTreeSensors(yield, bus);
    return parseSensorTree(tree, sensorsInfo);
}

std::vector<LabeledSensorInfo>
    TriggerFactory::getLabeledSensorsInfo(const SensorsInfo& sensorsInfo) const
{
    if (sensorsInfo.empty())
    {
        return {};
    }
    auto tree = utils::getSubTreeSensors(bus);
    return parseSensorTree(tree, sensorsInfo);
}

std::vector<LabeledSensorInfo>
    TriggerFactory::parseSensorTree(const std::vector<utils::SensorTree>& tree,
                                    const SensorsInfo& sensorsInfo)
{
    return utils::transform(sensorsInfo, [&tree](const auto& item) {
        const auto& [sensorPath, metadata] = item;
        auto found = std::find_if(tree.begin(), tree.end(),
                                  [path = sensorPath](const auto& x) {
            return x.first == path;
        });

        if (tree.end() != found)
        {
            const auto& [service, ifaces] = found->second.front();
            return LabeledSensorInfo(service, sensorPath, metadata);
        }
        throw std::runtime_error("Not found");
    });
}
