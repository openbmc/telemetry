#include "trigger_factory.hpp"

#include "trigger.hpp"

TriggerFactory::TriggerFactory(
    std::shared_ptr<sdbusplus::asio::connection> bus,
    std::shared_ptr<sdbusplus::asio::object_server> objServer) :
    bus(std::move(bus)),
    objServer(std::move(objServer))
{}

std::unique_ptr<interfaces::Trigger> TriggerFactory::make(
    const std::string& name, bool isDiscrete, bool logToJournal,
    bool logToRedfish, bool updateReport,
    const std::vector<std::pair<sdbusplus::message::object_path, std::string>>&
        sensors,
    const std::vector<std::string>& reportNames,
    const TriggerThresholdParams& thresholdParams,
    interfaces::TriggerManager& reportManager) const
{
    return std::make_unique<Trigger>(bus->get_io_context(), objServer, name,
                                     isDiscrete, logToJournal, logToRedfish,
                                     updateReport, sensors, reportNames,
                                     thresholdParams, reportManager);
}
