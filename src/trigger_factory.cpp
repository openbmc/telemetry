#include "trigger_factory.hpp"

#include "numeric_threshold.hpp"
#include "sensor.hpp"
#include "trigger.hpp"
#include "utils/dbus_mapper.hpp"

TriggerFactory::TriggerFactory(
    std::shared_ptr<sdbusplus::asio::connection> bus,
    std::shared_ptr<sdbusplus::asio::object_server> objServer,
    SensorCache& sensorCache) :
    bus(std::move(bus)),
    objServer(std::move(objServer)), sensorCache(sensorCache)
{}

std::unique_ptr<interfaces::Trigger> TriggerFactory::make(
    boost::asio::yield_context& yield, const std::string& name, bool isDiscrete,
    bool logToJournal, bool logToRedfish, bool updateReport,
    const std::vector<std::pair<sdbusplus::message::object_path, std::string>>&
        sensorPaths,
    const std::vector<std::string>& reportNames,
    const TriggerThresholdParams& thresholdParams,
    interfaces::TriggerManager& triggerManager) const
{
    if (isDiscrete)
    {
        throw std::runtime_error("Not implemented!");
    }

    auto [sensors, sensorNames] = getSensors(yield, sensorPaths);
    std::vector<std::shared_ptr<interfaces::Threshold>> thresholds;

    const auto& params =
        std::get<std::vector<numeric::ThresholdParam>>(thresholdParams);
    for (const auto& [type, dwellTime, direction, value] : params)
    {
        std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;

        thresholds.emplace_back(std::make_shared<NumericThreshold>(
            bus->get_io_context(), sensors, sensorNames, std::move(actions),
            std::chrono::milliseconds(dwellTime),
            static_cast<numeric::Direction>(direction), value));
    }

    return std::make_unique<Trigger>(
        bus->get_io_context(), objServer, name, isDiscrete, logToJournal,
        logToRedfish, updateReport, sensorPaths, reportNames, thresholdParams,
        std::move(thresholds), triggerManager);
}

std::pair<std::vector<std::shared_ptr<interfaces::Sensor>>,
          std::vector<std::string>>
    TriggerFactory::getSensors(
        boost::asio::yield_context& yield,
        const std::vector<std::pair<sdbusplus::message::object_path,
                                    std::string>>& sensorPaths) const
{
    auto tree = utils::getSubTreeSensors(yield, bus);

    std::vector<std::shared_ptr<interfaces::Sensor>> sensors;
    std::vector<std::string> sensorNames;
    for (const auto& [sensorPath, metadata] : sensorPaths)
    {
        auto found = std::find_if(
            tree.begin(), tree.end(),
            [&sensorPath](const auto& x) { return x.first == sensorPath; });
        if (found == tree.end())
        {
            throw std::runtime_error("Not found");
        }

        const auto& service = found->second[0].first;
        const auto& path = found->first;
        sensors.emplace_back(sensorCache.makeSensor<Sensor>(
            service, path, bus->get_io_context(), bus));
        if (metadata.empty())
        {
            sensorNames.emplace_back(sensorPath);
        }
        else
        {
            sensorNames.emplace_back(metadata);
        }
    }
    return {sensors, sensorNames};
}
