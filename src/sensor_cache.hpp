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
    std::shared_ptr<SensorType> makeSensor(std::string_view name,
                                           Args&&... args)
    {
        cleanupExpiredSensors();

        auto id = SensorType::makeId(name);
        auto it = sensors.find(id);

        if (it == sensors.end())
        {
            auto sensor = std::make_shared<SensorType>(
                std::move(id), std::forward<Args>(args)...);

            auto [kt, inserted] = sensors.insert(
                SensorsContainer::value_type(sensor->id(), std::move(sensor)));

            return std::static_pointer_cast<SensorType>(kt->second.lock());
        }

        return std::static_pointer_cast<SensorType>(it->second.lock());
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
