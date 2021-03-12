#include "discrete_threshold.hpp"

#include <phosphor-logging/log.hpp>

DiscreteThreshold::DiscreteThreshold(
    boost::asio::io_context& ioc,
    std::vector<std::shared_ptr<interfaces::Sensor>> sensorsIn,
    std::vector<std::string> sensorNames,
    std::vector<std::unique_ptr<interfaces::TriggerAction>> actionsIn,
    std::chrono::milliseconds dwellTimeIn, double thresholdValueIn,
    std::string name) :
    ioc(ioc),
    sensors(std::move(sensorsIn)), actions(std::move(actionsIn)),
    dwellTime(dwellTimeIn), thresholdValue(thresholdValueIn), name(name)
{
    details.reserve(sensors.size());
    for (size_t i = 0; i < sensors.size(); i++)
    {
        details.emplace_back(sensorNames[i], false, ioc);
    }
}

DiscreteThreshold::~DiscreteThreshold()
{}

void DiscreteThreshold::initialize()
{
    for (auto& sensor : sensors)
    {
        sensor->registerForUpdates(weak_from_this());
    }
}

DiscreteThreshold::ThresholdDetail&
    DiscreteThreshold::getDetails(interfaces::Sensor& sensor)
{
    auto it =
        std::find_if(sensors.begin(), sensors.end(),
                     [&sensor](const auto& x) { return &sensor == x.get(); });
    auto index = std::distance(sensors.begin(), it);
    return details.at(index);
}

void DiscreteThreshold::sensorUpdated(interfaces::Sensor& sensor,
                                      uint64_t timestamp)
{}

void DiscreteThreshold::sensorUpdated(interfaces::Sensor& sensor,
                                      uint64_t timestamp, double value)
{
    auto& [sensorName, dwell, timer] = getDetails(sensor);

    if (thresholdValue)
    {
        if (dwell && value != thresholdValue)
        {
            timer.cancel();
            dwell = false;
        }
        else if (value == thresholdValue)
        {
            startTimer(sensorName, timestamp, value, dwell, timer);
        }
    }
}

void DiscreteThreshold::startTimer(const std::string& sensorName,
                                   uint64_t timestamp, double value,
                                   bool& dwell,
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

void DiscreteThreshold::commit(const std::string& sensorName,
                               uint64_t timestamp, double value)
{
    for (const auto& action : actions)
    {
        action->commit(sensorName, timestamp, value);
    }
}

const char* DiscreteThreshold::getName() const
{
    return name.c_str();
}
