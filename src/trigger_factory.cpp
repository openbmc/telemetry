#include "trigger_factory.hpp"

#include "numeric_threshold.hpp"
#include "sensor.hpp"
#include "trigger.hpp"
#include "trigger_actions.hpp"
#include "utils/dbus_mapper.hpp"

TriggerFactory::TriggerFactory(
    std::shared_ptr<sdbusplus::asio::connection> bus,
    std::shared_ptr<sdbusplus::asio::object_server> objServer,
    SensorCache& sensorCache, interfaces::ReportManager& reportManager) :
    bus(std::move(bus)),
    objServer(std::move(objServer)), sensorCache(sensorCache),
    reportManager(reportManager)
{}

std::unique_ptr<interfaces::Trigger> TriggerFactory::make(
    boost::asio::yield_context& yield, const std::string& name,
    const bool isDiscrete, const bool logToJournal, const bool logToRedfish,
    const bool updateReport,
    const std::vector<sdbusplus::message::object_path>& sensorPaths,
    const std::vector<std::string>& reportNames,
    const TriggerThresholdParams& thresholdParams,
    interfaces::TriggerManager& triggerManager) const
{
    if (isDiscrete)
    {
        throw std::runtime_error("Not implemented!");
    }

    auto sensors = getSensors(yield, sensorPaths);
    std::vector<std::shared_ptr<interfaces::Threshold>> thresholds;

    const auto& params =
        std::get<std::vector<numeric::ThresholdParam>>(thresholdParams);
    for (const auto& [level, dwellTime, direction, value] : params)
    {
        std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;
        if (logToJournal)
        {
            actions.emplace_back(std::make_unique<LogToJournalAction>(
                static_cast<numeric::Level>(level),
                static_cast<numeric::Direction>(direction), value));
        }
        if (logToRedfish)
        {
            actions.emplace_back(std::make_unique<LogToRedfishAction>(
                static_cast<numeric::Level>(level), value));
        }
        if (updateReport)
        {
            actions.emplace_back(std::make_unique<UpdateReportAction>(
                reportManager, reportNames));
        }

        thresholds.emplace_back(std::make_shared<NumericThreshold>(
            bus->get_io_context(), sensors, std::move(actions),
            static_cast<numeric::Level>(level),
            std::chrono::milliseconds(dwellTime),
            static_cast<numeric::Direction>(direction), value));
    }

    return std::make_unique<Trigger>(
        bus->get_io_context(), objServer, name, isDiscrete, logToJournal,
        logToRedfish, updateReport, sensorPaths, reportNames, thresholdParams,
        std::move(thresholds), triggerManager);
}

std::vector<std::shared_ptr<interfaces::Sensor>> TriggerFactory::getSensors(
    boost::asio::yield_context& yield,
    const std::vector<sdbusplus::message::object_path>& sensorPaths) const
{
    auto tree = utils::getSubTreeSensors(yield, bus);

    std::vector<std::shared_ptr<interfaces::Sensor>> sensors;
    for (const auto& sensorPath : sensorPaths)
    {
        auto found = std::find_if(
            tree.begin(), tree.end(),
            [&sensorPath](const auto& x) { return x.first == sensorPath; });
        if (found == tree.end())
        {
            throw std::runtime_error("Not found");
        }

        sensors.emplace_back(sensorCache.makeSensor<Sensor>(
            found->second[0].first, found->first, bus->get_io_context(), bus));
    }

    return sensors;
}
