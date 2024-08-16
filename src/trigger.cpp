#include "trigger.hpp"

#include "messages/collect_trigger_id.hpp"
#include "messages/trigger_presence_changed_ind.hpp"
#include "trigger_manager.hpp"
#include "types/report_types.hpp"
#include "types/trigger_types.hpp"
#include "utils/contains.hpp"
#include "utils/conversion_trigger.hpp"
#include "utils/dbus_path_utils.hpp"
#include "utils/transform.hpp"

#include <phosphor-logging/log.hpp>

Trigger::Trigger(
    boost::asio::io_context& ioc,
    const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
    TriggerId&& idIn, const std::string& nameIn,
    const std::vector<TriggerAction>& triggerActionsIn,
    const std::shared_ptr<std::vector<std::string>> reportIdsIn,
    std::vector<std::shared_ptr<interfaces::Threshold>>&& thresholdsIn,
    interfaces::TriggerManager& triggerManager,
    interfaces::JsonStorage& triggerStorageIn,
    const interfaces::TriggerFactory& triggerFactory, Sensors sensorsIn) :
    id(std::move(idIn)),
    path(utils::pathAppend(utils::constants::triggerDirPath, *id)),
    name(nameIn), triggerActions(std::move(triggerActionsIn)),
    reportIds(std::move(reportIdsIn)), thresholds(std::move(thresholdsIn)),
    fileName(std::to_string(std::hash<std::string>{}(*id))),
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
                messages::Presence::Removed, *id, {}});
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
        }, [this](const auto&) { return persistent; });

        dbusIface.register_property_rw(
            "Thresholds", TriggerThresholdParams{},
            sdbusplus::vtable::property_::emits_change,
            [this, &triggerFactory](auto newVal, auto& oldVal) {
            auto newThresholdParams =
                std::visit(utils::ToLabeledThresholdParamConversion(), newVal);
            TriggerManager::verifyThresholdParams(newThresholdParams);
            triggerFactory.updateThresholds(thresholds, *id, triggerActions,
                                            reportIds, sensors,
                                            newThresholdParams);
            oldVal = std::move(newVal);
            return 1;
        }, [this](const auto&) {
            return fromLabeledThresholdParam(getLabeledThresholds());
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
        }, [this](const auto&) {
            return utils::fromLabeledSensorsInfo(getLabeledSensorInfo());
        });

        dbusIface.register_property_rw(
            "Reports", std::vector<sdbusplus::message::object_path>(),
            sdbusplus::vtable::property_::emits_change,
            [this](auto newVal, auto& oldVal) {
            auto newReportIds = utils::transform<std::vector>(
                newVal,
                [](const auto& path) { return utils::reportPathToId(path); });
            TriggerManager::verifyReportIds(newReportIds);
            *reportIds = newReportIds;
            messanger.send(messages::TriggerPresenceChangedInd{
                messages::Presence::Exist, *id, *reportIds});
            oldVal = std::move(newVal);
            return 1;
        }, [this](const auto&) {
            return utils::transform<std::vector>(*reportIds,
                                                 [](const auto& id) {
                return utils::pathAppend(utils::constants::reportDirPath, id);
            });
        });

        dbusIface.register_property_r(
            "Discrete", isDiscreate(), sdbusplus::vtable::property_::const_,
            [this](const auto& x) { return isDiscreate(); });

        dbusIface.register_property_rw(
            "Name", name, sdbusplus::vtable::property_::emits_change,
            [this](auto newVal, auto& oldVal) {
            if (newVal.length() > utils::constants::maxIdNameLength)
            {
                throw errors::InvalidArgument("Name", "Name is too long.");
            }
            name = oldVal = newVal;
            return 1;
        }, [this](const auto&) { return name; });

        dbusIface.register_property_r(
            "TriggerActions", std::vector<std::string>(),
            sdbusplus::vtable::property_::const_, [this](const auto&) {
            return utils::transform(triggerActions, [](const auto& action) {
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
            messanger.send(messages::CollectTriggerIdResp{*id});
        }
    });

    messanger.send(messages::TriggerPresenceChangedInd{
        messages::Presence::Exist, *id, *reportIds});
}

bool Trigger::storeConfiguration() const
{
    try
    {
        nlohmann::json data;

        auto labeledThresholdParams =
            std::visit(utils::ToLabeledThresholdParamConversion(),
                       fromLabeledThresholdParam(getLabeledThresholds()));

        data["Version"] = triggerVersion;
        data["Id"] = *id;
        data["Name"] = name;
        data["ThresholdParamsDiscriminator"] = labeledThresholdParams.index();
        data["TriggerActions"] = utils::transform(
            triggerActions,
            [](const auto& action) { return actionToString(action); });
        data["ThresholdParams"] =
            utils::labeledThresholdParamsToJson(labeledThresholdParams);
        data["ReportIds"] = *reportIds;
        data["Sensors"] = getLabeledSensorInfo();

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

std::vector<LabeledSensorInfo> Trigger::getLabeledSensorInfo() const
{
    return utils::transform(sensors, [](const auto& sensor) {
        return sensor->getLabeledSensorInfo();
    });
}

std::vector<LabeledThresholdParam> Trigger::getLabeledThresholds() const
{
    return utils::transform(thresholds, [](const auto& threshold) {
        return threshold->getThresholdParam();
    });
}

bool Trigger::isDiscreate() const
{
    const auto labeledThresholds = getLabeledThresholds();

    return utils::isFirstElementOfType<std::monostate>(labeledThresholds) ||
           utils::isFirstElementOfType<discrete::LabeledThresholdParam>(
               labeledThresholds);
}
