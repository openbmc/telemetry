#include "on_change_threshold.hpp"

#include <phosphor-logging/log.hpp>

OnChangeThreshold::OnChangeThreshold(
    Sensors sensorsIn,
    std::vector<std::unique_ptr<interfaces::TriggerAction>> actionsIn) :
    sensors(std::move(sensorsIn)),
    actions(std::move(actionsIn))
{}

void OnChangeThreshold::initialize()
{
    for (auto& sensor : sensors)
    {
        sensor->registerForUpdates(weak_from_this());
    }
    initialized = true;
}

void OnChangeThreshold::updateSensors(Sensors newSensors)
{
    Sensors oldSensors = sensors;

    for (const auto& sensor : newSensors)
    {
        auto it =
            std::find_if(oldSensors.begin(), oldSensors.end(),
                         [&sensor](const auto& s) { return sensor == s; });
        if (it != oldSensors.end())
        {
            oldSensors.erase(it);
            continue;
        }

        if (initialized)
        {
            sensor->registerForUpdates(weak_from_this());
        }
    }

    if (initialized)
    {
        for (auto& sensor : oldSensors)
        {
            sensor->unregisterFromUpdates(weak_from_this());
        }
    }

    sensors = std::move(newSensors);
}

void OnChangeThreshold::sensorUpdated(interfaces::Sensor& sensor,
                                      Milliseconds timestamp, double value)
{
    commit(sensor.getName(), timestamp, value);
}

void OnChangeThreshold::commit(const std::string& sensorName,
                               Milliseconds timestamp, double value)
{
    for (const auto& action : actions)
    {
        action->commit(sensorName, timestamp, value);
    }
}

LabeledThresholdParam OnChangeThreshold::getThresholdParam() const
{
    return {};
}
