#include "trigger.hpp"

#include "interfaces/types.hpp"
#include "utils/transform.hpp"

#include <phosphor-logging/log.hpp>

LabeledTriggerThresholdParams ToLabeledThresholdParamConversion::operator()(
    const std::vector<numeric::ThresholdParam>& arg) const
{
    return utils::transform(arg, [](const auto& thresholdParam) {
        const auto& [type, dwellTime, direction, thresholdValue] =
            thresholdParam;
        return numeric::LabeledThresholdParam(
            numeric::stringToType(type), dwellTime,
            numeric::stringToDirection(direction), thresholdValue);
    });
}

LabeledTriggerThresholdParams ToLabeledThresholdParamConversion::operator()(
    const std::vector<discrete::ThresholdParam>& arg) const
{
    return utils::transform(arg, [](const auto& thresholdParam) {
        const auto& [userId, severity, thresholdValue, dwellTime] =
            thresholdParam;
        return discrete::LabeledThresholdParam(
            userId, discrete::stringToSeverity(severity), thresholdValue,
            dwellTime);
    });
}

nlohmann::json labeledThresholdParamsToJson(
    const LabeledTriggerThresholdParams& labeledThresholdParams)
{
    return std::visit([](const auto& lt) { return nlohmann::json{lt}; },
                      labeledThresholdParams);
}

Trigger::Trigger(
    boost::asio::io_context& ioc,
    const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
    const std::string& nameIn, const bool isDiscreteIn,
    const bool logToJournalIn, const bool logToRedfishIn,
    const bool updateReportIn, const TriggerSensors& sensorsIn,
    const std::vector<std::string>& reportNamesIn,
    const TriggerThresholdParams& thresholdParamsIn,
    std::vector<std::shared_ptr<interfaces::Threshold>>&& thresholdsIn,
    interfaces::TriggerManager& triggerManager,
    interfaces::JsonStorage& triggerStorageIn) :
    name(nameIn),
    isDiscrete(isDiscreteIn), logToJournal(logToJournalIn),
    logToRedfish(logToRedfishIn), updateReport(updateReportIn),
    path(triggerDir + name), sensors(sensorsIn), reportNames(reportNamesIn),
    thresholdParams(thresholdParamsIn), thresholds(std::move(thresholdsIn)),
    fileName(std::to_string(std::hash<std::string>{}(name))),
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
        path, triggerIfaceName,
        [this, isDiscreteIn, logToJournalIn, logToRedfishIn,
         updateReportIn](auto& dbusIface) {
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

bool Trigger::storeConfiguration() const
{
    try
    {
        nlohmann::json data;

        data["Version"] = triggerVersion;
        data["Name"] = name;
        data["ThresholdParamsDiscriminator"] = thresholdParams.index();
        data["IsDiscrete"] = isDiscrete;
        data["LogToJournal"] = logToJournal;
        data["LogToRedfish"] = logToRedfish;
        data["UpdateReport"] = updateReport;

        data["ThresholdParams"] = labeledThresholdParamsToJson(
            std::visit(ToLabeledThresholdParamConversion(), thresholdParams));

        data["ReportNames"] = reportNames;

        data["Sensors"] = utils::transform(sensors, [](const auto& sensor) {
            const auto& [sensorPath, sensorMetadata] = sensor;
            return LabeledTriggerSensor(sensorPath, sensorMetadata);
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