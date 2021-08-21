#pragma once

#include "interfaces/sensor.hpp"

#include <boost/container/flat_map.hpp>
#include <boost/system/error_code.hpp>

#include <memory>
#include <string_view>

class SensorCache
{
  public:
    template <class SensorType, class... Args>
    std::shared_ptr<SensorType> makeSensor(std::string_view service,
                                           std::string_view path,
                                           Args&&... args)
    {
        cleanupExpiredSensors();

        auto id = SensorType::makeId(service, path);
        auto it = sensors.find(id);

        if (it == sensors.end())
        {
            auto sensor = std::make_shared<SensorType>(
                std::move(id), std::forward<Args>(args)...);

            sensors[sensor->id()] = sensor;

            return sensor;
        }

        return std::static_pointer_cast<SensorType>(it->second.lock());
    }

  private:
    using SensorsContainer =
        boost::container::flat_map<interfaces::Sensor::Id,
                                   std::weak_ptr<interfaces::Sensor>>;

    SensorsContainer sensors;

    SensorsContainer::iterator findExpiredSensor(SensorsContainer::iterator);
    void cleanupExpiredSensors();
};
