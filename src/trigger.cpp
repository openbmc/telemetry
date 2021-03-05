#include "trigger.hpp"

#include "interfaces/types.hpp"

Trigger::Trigger(
    boost::asio::io_context& ioc,
    const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
    const std::string& nameIn, const bool isDiscrete, const bool logToJournal,
    const bool logToRedfish, const bool updateReport,
    const std::vector<std::pair<sdbusplus::message::object_path, std::string>>&
        sensorsIn,
    const std::vector<std::string>& reportNamesIn,
    const TriggerThresholdParams& thresholdParamsIn,
    std::vector<std::shared_ptr<interfaces::Threshold>>&& thresholdsIn,
    interfaces::TriggerManager& triggerManager) :
    name(nameIn),
    path(triggerDir + name), persistent(false), sensors(sensorsIn),
    reportNames(reportNamesIn), thresholdParams(thresholdParamsIn),
    thresholds(std::move(thresholdsIn))
{
    deleteIface = objServer->add_unique_interface(
        path, deleteIfaceName, [this, &ioc, &triggerManager](auto& dbusIface) {
            dbusIface.register_method("Delete", [this, &ioc, &triggerManager] {
                boost::asio::post(ioc, [this, &triggerManager] {
                    triggerManager.removeTrigger(this);
                });
            });
        });

    triggerIface = objServer->add_unique_interface(
        path, triggerIfaceName,
        [this, isDiscrete, logToJournal, logToRedfish,
         updateReport](auto& dbusIface) {
            dbusIface.register_property_r(
                "Persistent", persistent,
                sdbusplus::vtable::property_::emits_change,
                [](const auto& x) { return x; });
            dbusIface.register_property_r(
                "Thresholds", thresholdParams,
                sdbusplus::vtable::property_::emits_change,
                [](const auto& x) { return x; });
            dbusIface.register_property_r(
                "Sensors", sensors, sdbusplus::vtable::property_::emits_change,
                [](const auto& x) { return x; });
            dbusIface.register_property_r(
                "ReportNames", reportNames,
                sdbusplus::vtable::property_::emits_change,
                [](const auto& x) { return x; });
            dbusIface.register_property_r("Discrete", isDiscrete,
                                          sdbusplus::vtable::property_::const_,
                                          [](const auto& x) { return x; });
            dbusIface.register_property_r("LogToJournal", logToJournal,
                                          sdbusplus::vtable::property_::const_,
                                          [](const auto& x) { return x; });
            dbusIface.register_property_r("LogToRedfish", logToRedfish,
                                          sdbusplus::vtable::property_::const_,
                                          [](const auto& x) { return x; });
            dbusIface.register_property_r("UpdateReport", updateReport,
                                          sdbusplus::vtable::property_::const_,
                                          [](const auto& x) { return x; });
        });

    for (const auto& threshold : thresholds)
    {
        threshold->initialize();
    }
}
