#include "trigger_manager.hpp"

TriggerManager::TriggerManager(
    std::unique_ptr<interfaces::TriggerFactory> triggerFactoryIn,
    std::unique_ptr<interfaces::JsonStorage> triggerStorageIn,
    const std::shared_ptr<sdbusplus::asio::object_server>& objServer) :
    triggerFactory(std::move(triggerFactoryIn)),
    triggerStorage(std::move(triggerStorageIn))
{
    managerIface = objServer->add_unique_interface(
        triggerManagerPath, triggerManagerIfaceName, [this](auto& iface) {
            iface.register_method(
                "AddTrigger",
                [this](
                    boost::asio::yield_context& yield, const std::string& name,
                    bool isDiscrete, bool logToJournal, bool logToRedfish,
                    bool updateReport,
                    const std::vector<std::pair<sdbusplus::message::object_path,
                                                std::string>>& sensors,
                    const std::vector<std::string>& reportNames,
                    const TriggerThresholdParams& thresholds) {
                    if (isDiscrete)
                    {
                        throw sdbusplus::exception::SdBusError(
                            static_cast<int>(std::errc::not_supported),
                            "Only numeric threshold is supported");
                    }

                    if (triggers.size() >= maxTriggers)
                    {
                        throw sdbusplus::exception::SdBusError(
                            static_cast<int>(std::errc::too_many_files_open),
                            "Reached maximal trigger count");
                    }

                    for (const auto& trigger : triggers)
                    {
                        if (trigger->getName() == name)
                        {
                            throw sdbusplus::exception::SdBusError(
                                static_cast<int>(std::errc::file_exists),
                                "Duplicate trigger");
                        }
                    }

                    triggers.emplace_back(triggerFactory->make(
                        yield, name, isDiscrete, logToJournal, logToRedfish,
                        updateReport, sensors, reportNames, thresholds, *this, *triggerStorage));
                    return triggers.back()->getPath();
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
