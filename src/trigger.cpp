#include "trigger.hpp"

#include "messages/collect_trigger_id.hpp"
#include "messages/trigger_presence_changed_ind.hpp"
#include "trigger_manager.hpp"
#include "types/report_types.hpp"
#include "types/trigger_types.hpp"
#include "utils/contains.hpp"
#include "utils/conversion_trigger.hpp"
#include "utils/transform.hpp"

#include <phosphor-logging/log.hpp>

Trigger::Trigger(
    boost::asio::io_context& ioc,
    const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
    const std::string& idIn, const std::string& nameIn,
    const std::vector<TriggerAction>& triggerActionsIn,
    const std::shared_ptr<std::vector<std::string>> reportIdsIn,
    std::vector<std::shared_ptr<interfaces::Threshold>>&& thresholdsIn,
    interfaces::TriggerManager& triggerManager,
    interfaces::JsonStorage& triggerStorageIn,
    const interfaces::TriggerFactory& triggerFactory, Sensors sensorsIn) :
    id(idIn),
    name(nameIn), triggerActions(std::move(triggerActionsIn)),
    path(triggerDir + id), reportIds(std::move(reportIdsIn)),
    thresholds(std::move(thresholdsIn)),
    fileName(std::to_string(std::hash<std::string>{}(id))),
    triggerStorage(triggerStorageIn), sensors(std::move(sensorsIn)),
    messanger(ioc)
{
    deleteIface = objServer->add_unique_interface(
        path, deleteIfaceName, [this, &ioc, &triggerManager](auto& dbusIface) {
            dbusIface.register_method("Delete", [this, &ioc, &triggerManager] {
                if (persistent)
                {
                    triggerStorage.remove(fileName);
                }
                messanger.send(messages::TriggerPresenceChangedInd{
                    messages::Presence::Removed, id, {}});
                boost::asio::post(ioc, [this, &triggerManager] {
                    triggerManager.removeTrigger(this);
                });
            });
        });

    triggerIface = objServer->add_unique_interface(
        path, triggerIfaceName, [this, &triggerFactory](auto& dbusIface) {
            persistent = storeConfiguration();
            dbusIface.register_property_rw(
                "Persistent", persistent,
                sdbusplus::vtable::property_::emits_change,
                [this](bool newVal, const auto&) {
                    if (newVal == persistent)
                    {
                        return 1;
                    }
                    if (newVal)
                    {
                        persistent = storeConfiguration();
                    }
                    else
                    {
                        triggerStorage.remove(fileName);
                        persistent = false;
                    }
                    return 1;
                },
                [this](const auto&) { return persistent; });

            dbusIface.register_property_rw(
                "Thresholds", TriggerThresholdParams{},
                sdbusplus::vtable::property_::emits_change,
                [this, &triggerFactory](auto newVal, auto& oldVal) {
                    auto newThresholdParams = std::visit(
                        utils::ToLabeledThresholdParamConversion(), newVal);
                    triggerFactory.updateThresholds(thresholds, triggerActions,
                                                    reportIds, sensors,
                                                    newThresholdParams);
                    oldVal = std::move(newVal);
                    return 1;
                },
                [this](const auto&) {
                    return fromLabeledThresholdParam(
                        utils::transform(thresholds, [](const auto& threshold) {
                            return threshold->getThresholdParam();
                        }));
                });

            dbusIface.register_property_rw(
                "Sensors", SensorsInfo{},
                sdbusplus::vtable::property_::emits_change,
                [this, &triggerFactory](auto newVal, auto& oldVal) {
                    auto labeledSensorInfo =
                        triggerFactory.getLabeledSensorsInfo(newVal);
                    triggerFactory.updateSensors(sensors, labeledSensorInfo);
                    for (const auto& threshold : thresholds)
                    {
                        threshold->updateSensors(sensors);
                    }
                    oldVal = std::move(newVal);
                    return 1;
                },
                [this](const auto&) {
                    return utils::fromLabeledSensorsInfo(
                        utils::transform(sensors, [](const auto& sensor) {
                            return sensor->getLabeledSensorInfo();
                        }));
                });

            dbusIface.register_property_rw(
                "ReportNames", std::vector<std::string>{},
                sdbusplus::vtable::property_::emits_change,
                [this](auto newVal, auto& oldVal) {
                    TriggerManager::verifyReportIds(newVal);
                    *reportIds = newVal;
                    messanger.send(messages::TriggerPresenceChangedInd{
                        messages::Presence::Exist, id, *reportIds});
                    oldVal = std::move(newVal);
                    return 1;
                },
                [this](const auto&) { return *reportIds; });

            dbusIface.register_property_r(
                "Discrete", false, sdbusplus::vtable::property_::const_,
                [this](const auto& x) {
                    return !std::holds_alternative<
                        numeric::LabeledThresholdParam>(
                        thresholds.back()->getThresholdParam());
                });

            dbusIface.register_property_rw(
                "Name", std::string(),
                sdbusplus::vtable::property_::emits_change,
                [this](auto newVal, auto& oldVal) {
                    name = oldVal = newVal;
                    return 1;
                },
                [this](const auto&) { return name; });

            dbusIface.register_property_r(
                "TriggerActions", std::vector<std::string>(),
                sdbusplus::vtable::property_::const_, [this](const auto&) {
                    return utils::transform(triggerActions,
                                            [](const auto& action) {
                                                return actionToString(action);
                                            });
                });
        });

    for (const auto& threshold : thresholds)
    {
        threshold->initialize();
    }

    messanger.on_receive<messages::CollectTriggerIdReq>(
        [this](const auto& msg) {
            if (utils::contains(*reportIds, msg.reportId))
            {
                messanger.send(messages::CollectTriggerIdResp{id});
            }
        });

    messanger.send(messages::TriggerPresenceChangedInd{
        messages::Presence::Exist, id, *reportIds});
}

bool Trigger::storeConfiguration() const
{
    try
    {
        nlohmann::json data;

        auto labeledThresholdParams =
            std::visit(utils::ToLabeledThresholdParamConversion(),
                       fromLabeledThresholdParam(utils::transform(
                           thresholds, [](const auto& threshold) {
                               return threshold->getThresholdParam();
                           })));

        data["Version"] = triggerVersion;
        data["Id"] = id;
        data["Name"] = name;
        data["ThresholdParamsDiscriminator"] = labeledThresholdParams.index();
        data["TriggerActions"] =
            utils::transform(triggerActions, [](const auto& action) {
                return actionToString(action);
            });
        data["ThresholdParams"] =
            utils::labeledThresholdParamsToJson(labeledThresholdParams);
        data["ReportIds"] = *reportIds;
        data["Sensors"] = utils::transform(sensors, [](const auto& sensor) {
            return sensor->getLabeledSensorInfo();
        });

        triggerStorage.store(fileName, data);
    }
    catch (const std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to store a trigger in storage",
            phosphor::logging::entry("EXCEPTION_MSG=%s", e.what()));
        return false;
    }
    return true;
}
