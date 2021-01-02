#include "numeric_threshold.hpp"

#include <phosphor-logging/log.hpp>

NumericThreshold::NumericThreshold(
    boost::asio::io_context& ioc,
    std::vector<std::shared_ptr<interfaces::Sensor>> sensorsIn,
    std::vector<std::string> sensorNames,
    std::vector<std::unique_ptr<interfaces::TriggerAction>> actionsIn,
    std::chrono::milliseconds dwellTimeIn, numeric::Direction direction,
    double thresholdValueIn) :
    ioc(ioc),
    sensors(std::move(sensorsIn)), actions(std::move(actionsIn)),
    dwellTime(dwellTimeIn), direction(direction),
    thresholdValue(thresholdValueIn)
{
    details.reserve(sensors.size());
    for (size_t i = 0; i < sensors.size(); i++)
    {
        details.emplace_back(sensorNames[i], thresholdValue, false, ioc);
    }
}

NumericThreshold::~NumericThreshold()
{}

void NumericThreshold::initialize()
{
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
    auto& [sensorName, prevValue, dwell, timer] = getDetails(sensor);
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
        startTimer(sensorName, timestamp, value, dwell, timer);
    }

    prevValue = value;
}

void NumericThreshold::startTimer(const std::string& sensorName,
                                  uint64_t timestamp, double value, bool& dwell,
                                  boost::asio::steady_timer& timer)
{
    if (dwellTime == std::chrono::milliseconds::zero())
    {
        commit(sensorName, timestamp, value);
    }
    else
    {
        dwell = true;
        timer.expires_after(dwellTime);
        timer.async_wait([this, sensorName, timestamp, value,
                          &dwell](const boost::system::error_code ec) {
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

void NumericThreshold::commit(const std::string& sensorName, uint64_t timestamp,
                              double value)
{
    for (const auto& action : actions)
    {
        action->commit(sensorName, timestamp, value);
    }
}
