#include "sensor_cache.hpp"

bool SensorCache::SensorIdCompare::operator()(
    const interfaces::Sensor::Id& left,
    const interfaces::Sensor::Id& right) const
{
    return std::tie(left.type, left.service, left.path) <
           std::tie(right.type, right.service, right.path);
}

std::optional<SensorCache::SensorsContainer::iterator>
    SensorCache::findExpiredSensor()
{
    auto it =
        std::find_if(sensors.begin(), sensors.end(),
                     [](const auto& item) { return item.second.expired(); });

    if (it == sensors.end())
    {
        return std::nullopt;
    }

    return it;
}

void SensorCache::cleanupExpiredSensors()
{
    while (auto expiredSensorIt = findExpiredSensor())
    {
        sensors.erase(*expiredSensorIt);
    }
}
