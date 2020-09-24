#include "sensor_cache.hpp"

SensorCache::SensorsContainer::iterator
    SensorCache::findExpiredSensor(SensorCache::SensorsContainer::iterator it)
{
    return std::find_if(it, sensors.end(),
                        [](const auto& item) { return item.second.expired(); });
}

void SensorCache::cleanupExpiredSensors()
{
    auto begin = sensors.begin();

    for (auto it = findExpiredSensor(begin); it != sensors.end();
         it = findExpiredSensor(begin))
    {
        begin = sensors.erase(it);
    }
}
