#include "on_change_threshold.hpp"

#include <phosphor-logging/log.hpp>

OnChangeThreshold::OnChangeThreshold(
    Sensors sensorsIn, std::vector<std::string> sensorNamesIn,
    std::vector<std::unique_ptr<interfaces::TriggerAction>> actionsIn) :
    sensors(std::move(sensorsIn)),
    sensorNames(std::move(sensorNamesIn)), actions(std::move(actionsIn))
{}

void OnChangeThreshold::initialize()
{
    for (auto& sensor : sensors)
    {
        sensor->registerForUpdates(weak_from_this());
    }
}

void OnChangeThreshold::sensorUpdated(interfaces::Sensor& sensor,
                                      uint64_t timestamp)
{}

void OnChangeThreshold::sensorUpdated(interfaces::Sensor& sensor,
                                      uint64_t timestamp, double value)
{
    auto it =
        std::find_if(sensors.begin(), sensors.end(),
                     [&sensor](const auto& x) { return &sensor == x.get(); });
    auto index = std::distance(sensors.begin(), it);
    commit(sensorNames.at(index), timestamp, value);
}

void OnChangeThreshold::commit(const std::string& sensorName,
                               uint64_t timestamp, double value)
{
    for (const auto& action : actions)
    {
        action->commit(sensorName, timestamp, value);
    }
}
