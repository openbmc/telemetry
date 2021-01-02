#include "trigger_factory.hpp"

#include "trigger.hpp"

TriggerFactory::TriggerFactory(
    std::shared_ptr<sdbusplus::asio::connection> bus,
    std::shared_ptr<sdbusplus::asio::object_server> objServer) :
    bus(std::move(bus)),
    objServer(std::move(objServer))
{}

std::unique_ptr<interfaces::Trigger> TriggerFactory::make(
    const std::string& name, const bool isDiscrete, const bool logToJournal,
    const bool logToRedfish, const bool updateReport,
    const std::vector<sdbusplus::message::object_path>& sensors,
    const std::vector<std::string>& reportNames,
    const TriggerThresholdParams& thresholds,
    interfaces::TriggerManager& reportManager) const
{
    return std::make_unique<Trigger>(bus->get_io_context(), objServer, name,
                                     isDiscrete, logToJournal, logToRedfish,
                                     updateReport, sensors, reportNames,
                                     thresholds, reportManager);
}
