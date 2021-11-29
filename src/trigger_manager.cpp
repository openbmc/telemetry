#include "trigger_manager.hpp"

#include "trigger.hpp"
#include "types/trigger_types.hpp"
#include "utils/conversion_trigger.hpp"
#include "utils/generate_id.hpp"
#include "utils/transform.hpp"

#include <phosphor-logging/log.hpp>

TriggerManager::TriggerManager(
    std::unique_ptr<interfaces::TriggerFactory> triggerFactoryIn,
    std::unique_ptr<interfaces::JsonStorage> triggerStorageIn,
    const std::shared_ptr<sdbusplus::asio::object_server>& objServer) :
    triggerFactory(std::move(triggerFactoryIn)),
    triggerStorage(std::move(triggerStorageIn))
{
    loadFromPersistent();

    managerIface = objServer->add_unique_interface(
        triggerManagerPath, triggerManagerIfaceName, [this](auto& iface) {
            iface.register_method(
                "AddTrigger",
                [this](boost::asio::yield_context& yield, const std::string& id,
                       const std::string& name,
                       const std::vector<std::string>& triggerActions,
                       const SensorsInfo& sensors,
                       const std::vector<std::string>& reportIds,
                       const TriggerThresholdParamsExt& thresholds) {
                    LabeledTriggerThresholdParams
                        labeledTriggerThresholdParams = std::visit(
                            utils::ToLabeledThresholdParamConversion(),
                            thresholds);

                    std::vector<LabeledSensorInfo> labeledSensorsInfo =
                        triggerFactory->getLabeledSensorsInfo(yield, sensors);

                    return addTrigger(id, name, triggerActions,
                                      labeledSensorsInfo, reportIds,
                                      labeledTriggerThresholdParams)
                        .getPath();
                });
        });
}

void TriggerManager::removeTrigger(const interfaces::Trigger* trigger)
{
    triggers.erase(
        std::remove_if(triggers.begin(), triggers.end(),
                       [trigger](const auto& x) { return trigger == x.get(); }),
        triggers.end());
}

void TriggerManager::verifyAddTrigger(const std::string& triggerId,
                                      const std::string& triggerName) const
{
    if (triggers.size() >= maxTriggers)
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::too_many_files_open),
            "Reached maximal trigger count");
    }

    verifyTriggerIdLength(triggerId);
    utils::verifyIdCharacters(triggerId);

    for (const auto& trigger : triggers)
    {
        if (trigger->getId() == triggerId)
        {
            throw sdbusplus::exception::SdBusError(
                static_cast<int>(std::errc::file_exists), "Duplicate trigger");
        }
    }
}

void TriggerManager::verifyTriggerIdLength(const std::string& triggerId)
{
    if (triggerId.length() > maxTriggerIdLength)
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::invalid_argument),
            "Trigger id exceeds maximum length");
    }
}

std::string TriggerManager::generateId(const std::string& prefix,
                                       const std::string& triggerName) const
{
    const auto existingTriggerIds = utils::transform(
        triggers, [](const auto& trigger) { return trigger->getId(); });

    return utils::generateId(prefix, triggerName, existingTriggerIds,
                             maxTriggerIdLength);
}

interfaces::Trigger& TriggerManager::addTrigger(
    const std::string& triggerIdIn, const std::string& triggerNameIn,
    const std::vector<std::string>& triggerActions,
    const std::vector<LabeledSensorInfo>& labeledSensorsInfo,
    const std::vector<std::string>& reportIds,
    const LabeledTriggerThresholdParams& labeledThresholdParams)
{
    std::string triggerName = triggerNameIn;
    if (triggerName.empty())
    {
        triggerName = triggerNameDefault;
    }

    std::string triggerId = generateId(triggerIdIn, triggerName);

    verifyAddTrigger(triggerId, triggerName);

    triggers.emplace_back(triggerFactory->make(
        triggerId, triggerName, triggerActions, reportIds, *this,
        *triggerStorage, labeledThresholdParams, labeledSensorsInfo));

    return *triggers.back();
}

void TriggerManager::loadFromPersistent()
{
    std::vector<interfaces::JsonStorage::FilePath> paths =
        triggerStorage->list();

    for (const auto& path : paths)
    {
        std::optional<nlohmann::json> data = triggerStorage->load(path);
        try
        {
            if (!data.has_value())
            {
                throw std::runtime_error("Empty storage");
            }
            size_t version = data->at("Version").get<size_t>();
            if (version != Trigger::triggerVersion)
            {
                throw std::runtime_error("Invalid version");
            }
            const std::string& id = data->at("Id").get_ref<std::string&>();
            const std::string& name = data->at("Name").get_ref<std::string&>();
            int thresholdParamsDiscriminator =
                data->at("ThresholdParamsDiscriminator").get<int>();
            const std::vector<std::string> triggerActions =
                data->at("TriggerActions").get<std::vector<std::string>>();

            LabeledTriggerThresholdParams labeledThresholdParams;
            if (0 == thresholdParamsDiscriminator)
            {
                labeledThresholdParams =
                    data->at("ThresholdParams")
                        .get<std::vector<numeric::LabeledThresholdParam>>();
            }
            else
            {
                labeledThresholdParams =
                    data->at("ThresholdParams")
                        .get<std::vector<discrete::LabeledThresholdParam>>();
            }

            auto reportIds =
                data->at("ReportIds").get<std::vector<std::string>>();

            auto labeledSensorsInfo =
                data->at("Sensors").get<std::vector<LabeledSensorInfo>>();

            addTrigger(id, name, triggerActions, labeledSensorsInfo, reportIds,
                       labeledThresholdParams);
        }
        catch (const std::exception& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Failed to load trigger from storage",
                phosphor::logging::entry(
                    "FILENAME=%s",
                    static_cast<std::filesystem::path>(path).c_str()),
                phosphor::logging::entry("EXCEPTION_MSG=%s", e.what()));
            triggerStorage->remove(path);
        }
    }
}
