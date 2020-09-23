#pragma once

#include "interfaces/sensor.hpp"
#include "utils/log.hpp"

#include <boost/container/flat_map.hpp>
#include <boost/system/error_code.hpp>

#include <iostream>
#include <memory>
#include <string_view>

class SensorCache
{
    struct SensorIdCompare
    {
        bool operator()(const interfaces::Sensor::Id& left,
                        const interfaces::Sensor::Id& right) const
        {
            return std::tie(left.type, left.name) <
                   std::tie(right.type, right.name);
        }
    };

  public:
    template <class SensorType, class... Args>
    std::shared_ptr<SensorType> makeSensor(const interfaces::Sensor::Id& id,
                                           Args&&... args)
    {
        cleanupExpiredSensors();

        auto it = sensors.find(id);

        if (it == sensors.end())
        {
            auto sensor =
                std::make_shared<SensorType>(std::forward<Args>(args)...);

            auto [kt, inserted] =
                sensors.insert(std::make_pair(sensor->id(), sensor));

            return kt->second.lock();
        }

        return it->second.lock();
    }

  private:
    using SensorsContainer =
        boost::container::flat_map<interfaces::Sensor::Id,
                                   std::weak_ptr<interfaces::Sensor>,
                                   SensorIdCompare>;

    SensorsContainer sensors;

    std::optional<SensorsContainer::iterator> findExpiredSensor()
    {
        auto it =
            std::find_if(sensors.begin(), sensors.end(), [](const auto& item) {
                return item.second.expired();
            });

        if (it == sensors.end())
        {
            return std::nullopt;
        }

        return it;
    }

    void cleanupExpiredSensors()
    {
        while (auto expiredSensorIt = findExpiredSensor())
        {
            sensors.erase(*expiredSensorIt);
        }
    }
};
