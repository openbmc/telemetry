#include "numeric_threshold.hpp"

#include <phosphor-logging/log.hpp>

NumericThreshold::NumericThreshold(
    boost::asio::io_context& ioc, Sensors sensorsIn,
    std::vector<std::unique_ptr<interfaces::TriggerAction>> actionsIn,
    Milliseconds dwellTimeIn, numeric::Direction directionIn,
    double thresholdValueIn, numeric::Type typeIn) :
    ioc(ioc),
    actions(std::move(actionsIn)), dwellTime(dwellTimeIn),
    direction(directionIn), thresholdValue(thresholdValueIn), type(typeIn)
{
    for (const auto& sensor : sensorsIn)
    {
        sensorDetails.emplace(sensor, makeDetails(sensor->getName()));
    }
}

void NumericThreshold::initialize()
{
    ThresholdMixin().initialize(this);
}

void NumericThreshold::updateSensors(Sensors newSensors)
{
    ThresholdMixin().updateSensors(this, std::move(newSensors));
}

NumericThreshold::ThresholdDetail&
    NumericThreshold::getDetails(const interfaces::Sensor& sensor)
{
    return ThresholdMixin().getDetails(this, sensor);
}

std::shared_ptr<NumericThreshold::ThresholdDetail>
    NumericThreshold::makeDetails(const std::string& sensorName)
{
    return std::make_shared<ThresholdDetail>(sensorName, thresholdValue, false,
                                             ioc);
}

void NumericThreshold::sensorUpdated(interfaces::Sensor& sensor,
                                     Milliseconds timestamp)
{}

void NumericThreshold::sensorUpdated(interfaces::Sensor& sensor,
                                     Milliseconds timestamp, double value)
{
    auto& details = getDetails(sensor);
    auto& [sensorName, prevValue, dwell, timer] = details;
    bool decreasing = thresholdValue < prevValue && thresholdValue > value;
    bool increasing = thresholdValue > prevValue && thresholdValue < value;

    if (dwell && (increasing || decreasing))
    {
        timer.cancel();
        dwell = false;
    }
    if ((direction == numeric::Direction::decreasing && decreasing) ||
        (direction == numeric::Direction::increasing && increasing) ||
        (direction == numeric::Direction::either && (increasing || decreasing)))
    {
        startTimer(details, timestamp, value);
    }

    prevValue = value;
}

void NumericThreshold::startTimer(NumericThreshold::ThresholdDetail& details,
                                  Milliseconds timestamp, double value)
{
    auto& [sensorName, prevValue, dwell, timer] = details;
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

void NumericThreshold::commit(const std::string& sensorName,
                              Milliseconds timestamp, double value)
{
    for (const auto& action : actions)
    {
        action->commit(sensorName, timestamp, value);
    }
}

LabeledThresholdParam NumericThreshold::getThresholdParam() const
{
    return numeric::LabeledThresholdParam(type, dwellTime.count(), direction,
                                          thresholdValue);
}
