#include "trigger.hpp"

#include "trigger_manager.hpp"
#include "types/report_types.hpp"
#include "types/trigger_types.hpp"
#include "utils/conversion_trigger.hpp"
#include "utils/transform.hpp"

#include <phosphor-logging/log.hpp>

Trigger::Trigger(
    boost::asio::io_context& ioc,
    const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
    const std::string& idIn, const std::string& nameIn,
    const std::vector<std::string>& triggerActionsIn,
    const std::vector<std::string>& reportNamesIn,
    const std::vector<LabeledSensorInfo>& LabeledSensorsInfoIn,
    const LabeledTriggerThresholdParams& labeledThresholdParamsIn,
    std::vector<std::shared_ptr<interfaces::Threshold>>&& thresholdsIn,
    interfaces::TriggerManager& triggerManager,
    interfaces::JsonStorage& triggerStorageIn) :
    id(idIn),
    name(nameIn), triggerActions(std::move(triggerActionsIn)),
    path(triggerDir + id), reportNames(reportNamesIn),
    labeledSensorsInfo(LabeledSensorsInfoIn),
    labeledThresholdParams(labeledThresholdParamsIn),
    thresholds(std::move(thresholdsIn)),
    fileName(std::to_string(std::hash<std::string>{}(id))),
    triggerStorage(triggerStorageIn)
{
    deleteIface = objServer->add_unique_interface(
        path, deleteIfaceName, [this, &ioc, &triggerManager](auto& dbusIface) {
            dbusIface.register_method("Delete", [this, &ioc, &triggerManager] {
                if (persistent)
                {
                    triggerStorage.remove(fileName);
                }
                boost::asio::post(ioc, [this, &triggerManager] {
                    triggerManager.removeTrigger(this);
                });
            });
        });

    triggerIface = objServer->add_unique_interface(
        path, triggerIfaceName, [this](auto& dbusIface) {
            persistent = storeConfiguration();
            dbusIface.register_property_rw(
                "Persistent", persistent,
                sdbusplus::vtable::property_::emits_change,
                [this](bool newVal, const auto&) {
                    if (newVal == persistent)
                    {
                        return true;
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
                    return true;
                },
                [this](const auto&) { return persistent; });

            dbusIface.register_property_r(
                "Thresholds", TriggerThresholdParams{},
                sdbusplus::vtable::property_::emits_change,
                [this](const auto&) {
                    return std::visit(
                        utils::FromLabeledThresholdParamConversion(),
                        labeledThresholdParams);
                });

            dbusIface.register_property_r(
                "Sensors", SensorsInfo{},
                sdbusplus::vtable::property_::emits_change,
                [this](const auto&) {
                    return utils::fromLabeledSensorsInfo(labeledSensorsInfo);
                });

            dbusIface.register_property_r(
                "ReportNames", reportNames,
                sdbusplus::vtable::property_::emits_change,
                [](const auto& x) { return x; });

            dbusIface.register_property_r(
                "Discrete", false, sdbusplus::vtable::property_::const_,
                [this](const auto& x) {
                    return isTriggerThresholdDiscrete(labeledThresholdParams);
                });

            dbusIface.register_property_rw(
                "Name", std::string(),
                sdbusplus::vtable::property_::emits_change,
                [this](auto newVal, auto& oldVal) {
                    TriggerManager::verifyTriggerNameLength(newVal);
                    name = oldVal = newVal;
                    return true;
                },
                [this](const auto&) { return name; });

            dbusIface.register_property_r("TriggerActions", triggerActions,
                                          sdbusplus::vtable::property_::const_,
                                          [this](const auto& x) { return x; });
        });

    for (const auto& threshold : thresholds)
    {
        threshold->initialize();
    }
}

bool Trigger::storeConfiguration() const
{
    try
    {
        nlohmann::json data;

        data["Version"] = triggerVersion;
        data["Id"] = id;
        data["Name"] = name;
        data["ThresholdParamsDiscriminator"] = labeledThresholdParams.index();
        data["TriggerActions"] = triggerActions;
        data["ThresholdParams"] =
            utils::labeledThresholdParamsToJson(labeledThresholdParams);
        data["ReportNames"] = reportNames;
        data["Sensors"] = labeledSensorsInfo;

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
