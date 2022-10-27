#include "discrete_threshold.hpp"

#include "utils/conversion_trigger.hpp"
#include "utils/to_short_enum.hpp"

#include <phosphor-logging/log.hpp>

DiscreteThreshold::DiscreteThreshold(
    boost::asio::io_context& ioc, const std::string& triggerIdIn,
    Sensors sensorsIn,
    std::vector<std::unique_ptr<interfaces::TriggerAction>> actionsIn,
    Milliseconds dwellTimeIn, const std::string& thresholdValueIn,
    const std::string& nameIn, const discrete::Severity severityIn,
    std::unique_ptr<interfaces::Clock> clockIn) :
    ioc(ioc),
    triggerId(triggerIdIn), actions(std::move(actionsIn)),
    dwellTime(dwellTimeIn), thresholdValue(thresholdValueIn),
    numericThresholdValue(utils::stodStrict(thresholdValue)),
    severity(severityIn), name(getNonEmptyName(nameIn)),
    clock(std::move(clockIn))
{
    for (const auto& sensor : sensorsIn)
    {
        sensorDetails.emplace(sensor, makeDetails(sensor->getName()));
    }
}

void DiscreteThreshold::initialize()
{
    ThresholdOperations::initialize(this);
}

void DiscreteThreshold::updateSensors(Sensors newSensors)
{
    ThresholdOperations::updateSensors(this, std::move(newSensors));
}

DiscreteThreshold::ThresholdDetail&
    DiscreteThreshold::getDetails(const interfaces::Sensor& sensor)
{
    return ThresholdOperations::getDetails(this, sensor);
}

std::shared_ptr<DiscreteThreshold::ThresholdDetail>
    DiscreteThreshold::makeDetails(const std::string& sensorName)
{
    return std::make_shared<ThresholdDetail>(sensorName, ioc);
}

void DiscreteThreshold::sensorUpdated(interfaces::Sensor& sensor,
                                      Milliseconds timestamp, double value)
{
    auto& details = getDetails(sensor);
    auto& dwell = details.dwell;
    auto& timer = details.timer;

    if (dwell && value != numericThresholdValue)
    {
        timer.cancel();
        dwell = false;
    }
    else if (value == numericThresholdValue)
    {
        startTimer(details, value);
    }
}

void DiscreteThreshold::startTimer(DiscreteThreshold::ThresholdDetail& details,
                                   double value)
{
    auto& sensorName = details.getSensorName();
    auto& dwell = details.dwell;
    auto& timer = details.timer;

    if (dwellTime == Milliseconds::zero())
    {
        commit(sensorName, value);
    }
    else
    {
        dwell = true;
        timer.expires_after(dwellTime);
        timer.async_wait([this, &sensorName, &dwell,
                          value](const boost::system::error_code ec) {
            if (ec)
            {
                phosphor::logging::log<phosphor::logging::level::DEBUG>(
                    "Timer has been canceled");
                return;
            }
            commit(sensorName, value);
            dwell = false;
        });
    }
}

void DiscreteThreshold::commit(const std::string& sensorName, double value)
{
    Milliseconds timestamp = clock->systemTimestamp();
    for (const auto& action : actions)
    {
        action->commit(triggerId, std::cref(name), sensorName, timestamp,
                       thresholdValue);
    }
}

LabeledThresholdParam DiscreteThreshold::getThresholdParam() const
{
    return discrete::LabeledThresholdParam(name, severity, dwellTime.count(),
                                           thresholdValue);
}

std::string DiscreteThreshold::getNonEmptyName(const std::string& nameIn) const
{
    if (nameIn.empty())
    {
        return std::string(utils::toShortEnum(utils::enumToString(severity))) +
               " condition";
    }
    return nameIn;
}
