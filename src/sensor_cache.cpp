#include "sensor_cache.hpp"

bool SensorCache::SensorIdCompare::operator()(
    const interfaces::Sensor::Id& left,
    const interfaces::Sensor::Id& right) const
{
    return std::tie(left.type, left.service, left.path) <
           std::tie(right.type, right.service, right.path);
}

SensorCache::SensorsContainer::iterator SensorCache::findExpiredSensor()
{
    return std::find_if(sensors.begin(), sensors.end(),
                        [](const auto& item) { return item.second.expired(); });
}

void SensorCache::cleanupExpiredSensors()
{
    for (auto it = findExpiredSensor(); it != sensors.end();
         it = findExpiredSensor())
    {
        sensors.erase(it);
    }
}
