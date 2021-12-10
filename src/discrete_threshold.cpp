#include "discrete_threshold.hpp"

#include <phosphor-logging/log.hpp>

DiscreteThreshold::DiscreteThreshold(
    boost::asio::io_context& ioc, Sensors sensorsIn,
    std::vector<std::unique_ptr<interfaces::TriggerAction>> actionsIn,
    Milliseconds dwellTimeIn, double thresholdValueIn,
    const std::string& nameIn, const discrete::Severity severityIn) :
    ioc(ioc),
    actions(std::move(actionsIn)), dwellTime(dwellTimeIn),
    thresholdValue(thresholdValueIn), name(nameIn), severity(severityIn)
{
    for (const auto& sensor : sensorsIn)
    {
        sensorDetails.emplace(sensor, makeDetails(sensor->getName()));
    }
}

void DiscreteThreshold::initialize()
{
    ThresholdMixin().initialize(this);
}

void DiscreteThreshold::updateSensors(Sensors newSensors)
{
    ThresholdMixin().updateSensors(this, std::move(newSensors));
}

DiscreteThreshold::ThresholdDetail&
    DiscreteThreshold::getDetails(const interfaces::Sensor& sensor)
{
    return ThresholdMixin().getDetails(this, sensor);
}

std::shared_ptr<DiscreteThreshold::ThresholdDetail>
    DiscreteThreshold::makeDetails(const std::string& sensorName)
{
    return std::make_shared<ThresholdDetail>(sensorName, false, ioc);
}

void DiscreteThreshold::sensorUpdated(interfaces::Sensor& sensor,
                                      Milliseconds timestamp)
{}

void DiscreteThreshold::sensorUpdated(interfaces::Sensor& sensor,
                                      Milliseconds timestamp, double value)
{
    auto& details = getDetails(sensor);
    auto& [sensorName, dwell, timer] = details;

    if (thresholdValue)
    {
        if (dwell && value != thresholdValue)
        {
            timer.cancel();
            dwell = false;
        }
        else if (value == thresholdValue)
        {
            startTimer(details, timestamp, value);
        }
    }
}

void DiscreteThreshold::startTimer(DiscreteThreshold::ThresholdDetail& details,
                                   Milliseconds timestamp, double value)
{
    auto& [sensorName, dwell, timer] = details;
    if (dwellTime == Milliseconds::zero())
    {
        commit(sensorName, timestamp, value);
    }
    else
    {
        dwell = true;
        timer.expires_after(dwellTime);
        timer.async_wait([this, &sensorName, &dwell, timestamp,
                          value](const boost::system::error_code ec) {
            if (ec)
            {
                phosphor::logging::log<phosphor::logging::level::DEBUG>(
                    "Timer has been canceled");
                return;
            }
            commit(sensorName, timestamp, value);
            dwell = false;
        });
    }
}

void DiscreteThreshold::commit(const std::string& sensorName,
                               Milliseconds timestamp, double value)
{
    for (const auto& action : actions)
    {
        action->commit(sensorName, timestamp, value);
    }
}

LabeledThresholdParam DiscreteThreshold::getThresholdParam() const
{
    return discrete::LabeledThresholdParam(name, severity, dwellTime.count(),
                                           std::to_string(thresholdValue));
}
