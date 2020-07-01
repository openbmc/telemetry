#pragma once

#include "interfaces/sensor.hpp"
#include "log.hpp"

#include <boost/container/flat_map.hpp>
#include <boost/system/error_code.hpp>

#include <iostream>
#include <memory>
#include <string_view>

namespace core
{

class SensorCache
{
  public:
    using AllSensors =
        boost::container::flat_map<interfaces::Sensor::GlobalId,
                                   std::weak_ptr<interfaces::Sensor>>;

    template <typename SensorType, typename... Args>
    static std::shared_ptr<interfaces::Sensor>
        make(std::unique_ptr<const typename SensorType::Id> id, Args&&... args)
    {
        cleanup();

        auto it = sensors.find(id->globalId());

        if (it == sensors.end())
        {
            auto sensor = std::make_shared<SensorType>(
                std::move(id), std::forward<Args>(args)...);

            sensors.emplace(sensor->globalId(), sensor);

            return sensor;
        }
        else
        {
            return it->second.lock();
        }

        return {};
    }

    static void cleanup()
    {
        auto it = sensors.begin();
        while (it != sensors.end())
        {
            if (it->second.expired())
            {
                LOG_DEBUG_T("SensorCache") << "Removing dangling pointer";
                it = sensors.erase(it);
                continue;
            }
            it++;
        }
    }

  private:
    static AllSensors sensors;
};

} // namespace core

namespace details
{
template <class T>
std::ostream& streamPtr(std::ostream& o, const std::shared_ptr<const T>& s)
{
    if (s)
    {
        return o << *s;
    }
    return o << s;
}
} // namespace details

inline std::ostream& operator<<(std::ostream& o,
                                const core::interfaces::Sensor& s)
{
    return o << s.globalId();
}

inline std::ostream&
    operator<<(std::ostream& o,
               const std::shared_ptr<const core::interfaces::Sensor>& s)
{
    return details::streamPtr(o, s);
}
