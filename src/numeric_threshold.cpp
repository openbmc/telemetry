#include "numeric_threshold.hpp"

#include <phosphor-logging/log.hpp>

NumericThreshold::NumericThreshold(
    boost::asio::io_context& ioc,
    std::vector<std::shared_ptr<interfaces::Sensor>> sensorsIn,
    std::vector<std::unique_ptr<interfaces::TriggerAction>> actionsIn,
    NumericThresholdType thresholdTypeIn, std::chrono::milliseconds dwellTimeIn,
    NumericActivationType activationType, double thresholdValueIn) :
    timer(ioc),
    sensors(std::move(sensorsIn)), actions(std::move(actionsIn)),
    thresholdType(thresholdTypeIn), dwellTime(dwellTimeIn),
    activationType(activationType), thresholdValue(thresholdValueIn)
{}

NumericThreshold::~NumericThreshold()
{
    timer.cancel();
}

void NumericThreshold::initialize()
{
    details.reserve(sensors.size());
    details.assign(sensors.size(), {thresholdValue, false});

    for (auto& sensor : sensors)
    {
        sensor->registerForUpdates(weak_from_this());
    }
}

NumericThreshold::ThresholdDetail&
    NumericThreshold::getDetails(interfaces::Sensor& sensor)
{
    auto it =
        std::find_if(sensors.begin(), sensors.end(),
                     [&sensor](const auto& x) { return &sensor == x.get(); });
    auto index = std::distance(sensors.begin(), it);
    return details.at(index);
}

void NumericThreshold::sensorUpdated(interfaces::Sensor& sensor,
                                     uint64_t timestamp)
{}

void NumericThreshold::sensorUpdated(interfaces::Sensor& sensor,
                                     uint64_t timestamp, double value)
{
    auto& [prevValue, dwell] = getDetails(sensor);

    if (activationType == NumericActivationType::decreasing)
    {
        if (dwell)
        {
            if (thresholdValue < value)
            {
                timer.cancel();
                dwell = false;
            }
        }
        else if (thresholdValue < prevValue && thresholdValue > value)
        {
            startTimer(timestamp, value, dwell);
        }
    }
    else if (activationType == NumericActivationType::increasing)
    {
        if (dwell)
        {
            if (thresholdValue > value)
            {
                timer.cancel();
                dwell = false;
            }
        }
        else if (thresholdValue > prevValue && thresholdValue < value)
        {
            startTimer(timestamp, value, dwell);
        }
    }
    else if (activationType == NumericActivationType::either)
    {
        if (dwell)
        {
            if ((thresholdValue > prevValue && thresholdValue < value) ||
                (thresholdValue < prevValue && thresholdValue > value))
            {
                timer.cancel();
                dwell = false;
            }
        }
        if (!dwell && ((thresholdValue > prevValue && thresholdValue < value) ||
                       (thresholdValue < prevValue && thresholdValue > value)))
        {
            startTimer(timestamp, value, dwell);
        }
    }

    prevValue = value;
}

void NumericThreshold::log(uint64_t timestamp, double value)
{
    for (const auto& action : actions)
    {
        action->commit(timestamp, value);
    }
}

void NumericThreshold::startTimer(uint64_t timestamp, double value, bool& dwell)
{
    if (dwellTime == std::chrono::milliseconds::zero())
    {
        log(timestamp, value);
    }
    else
    {
        dwell = true;
        timer.expires_after(dwellTime);
        timer.async_wait([this, timestamp, value,
                          &dwell](const boost::system::error_code ec) {
            dwell = false;
            if (ec)
            {
                phosphor::logging::log<phosphor::logging::level::DEBUG>(
                    "Timer has been canceled");
                return;
            }
            log(timestamp, value);
        });
    }
}
