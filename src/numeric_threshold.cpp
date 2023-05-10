#include "numeric_threshold.hpp"

#include <phosphor-logging/log.hpp>

NumericThreshold::NumericThreshold(
    boost::asio::io_context& ioc, const std::string& triggerIdIn,
    Sensors sensorsIn,
    std::vector<std::unique_ptr<interfaces::TriggerAction>> actionsIn,
    Milliseconds dwellTimeIn, numeric::Direction directionIn,
    double thresholdValueIn, numeric::Type typeIn,
    std::unique_ptr<interfaces::Clock> clockIn) :
    ioc(ioc),
    triggerId(triggerIdIn), actions(std::move(actionsIn)),
    dwellTime(dwellTimeIn), direction(directionIn),
    thresholdValue(thresholdValueIn), type(typeIn), clock(std::move(clockIn))
{
    for (const auto& sensor : sensorsIn)
    {
        sensorDetails.emplace(sensor, makeDetails(sensor->getName()));
    }
}

void NumericThreshold::initialize()
{
    ThresholdOperations::initialize(this);
}

void NumericThreshold::updateSensors(Sensors newSensors)
{
    ThresholdOperations::updateSensors(this, std::move(newSensors));
}

NumericThreshold::ThresholdDetail&
    NumericThreshold::getDetails(const interfaces::Sensor& sensor)
{
    return ThresholdOperations::getDetails(this, sensor);
}

std::shared_ptr<NumericThreshold::ThresholdDetail>
    NumericThreshold::makeDetails(const std::string& sensorName)
{
    return std::make_shared<ThresholdDetail>(sensorName, ioc);
}

void NumericThreshold::sensorUpdated(interfaces::Sensor& sensor,
                                     Milliseconds timestamp, double value)
{
    auto& details = getDetails(sensor);
    auto& prevValue = details.prevValue;
    auto& prevDirection = details.prevDirection;
    auto& dwell = details.dwell;
    auto& timer = details.timer;

    if (!prevValue)
    {
        prevValue = value;
        return;
    }

    bool crossedDecreasing = thresholdValue < prevValue &&
                             thresholdValue > value;
    bool crossedIncreasing = thresholdValue > prevValue &&
                             thresholdValue < value;

    if (!crossedDecreasing && !crossedIncreasing && thresholdValue == prevValue)
    {
        crossedDecreasing = prevDirection == numeric::Direction::decreasing &&
                            thresholdValue > value;
        crossedIncreasing = prevDirection == numeric::Direction::increasing &&
                            thresholdValue < value;
    }

    if (dwell && (crossedIncreasing || crossedDecreasing))
    {
        timer.cancel();
        dwell = false;
    }
    if ((direction == numeric::Direction::decreasing && crossedDecreasing) ||
        (direction == numeric::Direction::increasing && crossedIncreasing) ||
        (direction == numeric::Direction::either &&
         (crossedIncreasing || crossedDecreasing)))
    {
        startTimer(details, value);
    }

    prevDirection = value > prevValue   ? numeric::Direction::increasing
                    : value < prevValue ? numeric::Direction::decreasing
                                        : numeric::Direction::either;
    prevValue = value;
}

void NumericThreshold::startTimer(NumericThreshold::ThresholdDetail& details,
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

void NumericThreshold::commit(const std::string& sensorName, double value)
{
    Milliseconds timestamp = clock->systemTimestamp();
    for (const auto& action : actions)
    {
        action->commit(triggerId, std::nullopt, sensorName, timestamp, value);
    }
}

LabeledThresholdParam NumericThreshold::getThresholdParam() const
{
    return numeric::LabeledThresholdParam(type, dwellTime.count(), direction,
                                          thresholdValue);
}
