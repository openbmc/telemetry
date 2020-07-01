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

class Sensor :
    public interfaces::Sensor,
    public std::enable_shared_from_this<Sensor>
{
  public:
    explicit Sensor(std::shared_ptr<const Id> id) : id_(std::move(id))
    {
        LOG_DEBUG_T(id_) << "core::Sensor Constructor";
    }

    virtual ~Sensor()
    {
        LOG_DEBUG_T(id_) << "core::Sensor ~Sensor";
    }

    std::shared_ptr<const Id> id() const override final
    {
        return id_;
    }

    static inline void notFoundError(const ReadCallback& callback)
    {
        LOG_ERROR << __PRETTY_FUNCTION__;
        constexpr auto e = boost::system::errc::no_such_device;
        callback(boost::system::errc::make_error_code(e), 0);
    }

    static inline void readFailedError(const ReadCallback& callback)
    {
        LOG_ERROR << __PRETTY_FUNCTION__;
        constexpr auto e = boost::system::errc::io_error;
        callback(boost::system::errc::make_error_code(e), 0);
    }

    virtual void async_read(interfaces::Sensor::ReadCallback&& callback) = 0;
    virtual void
        registerForUpdates(interfaces::Sensor::ReadCallback&& callback) = 0;

  private:
    const std::shared_ptr<const Id> id_;
};

class SensorCache
{
    struct IdCompare
    {
        bool operator()(
            const std::weak_ptr<const interfaces::Sensor::Id>& lhs,
            const std::weak_ptr<const interfaces::Sensor::Id>& rhs) const
        {
            auto lhs_ = lhs.lock();
            auto rhs_ = rhs.lock();

            if (lhs_ && rhs_)
            {
                return (*lhs_) < (*rhs_);
            }

            throw std::runtime_error("Inconsistency in Sensors set");
            return false;
        }
    };

  public:
    using AllSensors =
        boost::container::flat_map<std::weak_ptr<const interfaces::Sensor::Id>,
                                   std::weak_ptr<interfaces::Sensor>,
                                   IdCompare>;

    template <typename SensorType, typename Id, typename... Args>
    static typename std::enable_if<
        std::is_base_of<interfaces::Sensor::Id, Id>::value,
        std::shared_ptr<interfaces::Sensor>>::type
        make(std::shared_ptr<const Id> sensorId, Args&&... args)
    {
        // Remove any dangling pointers
        cleanup();

        auto it = sensors.find(std::weak_ptr<const interfaces::Sensor::Id>(
            std::static_pointer_cast<const interfaces::Sensor::Id>(sensorId)));

        if (it == sensors.end())
        {
            auto sensor = std::make_shared<SensorType>(
                sensorId, std::forward<Args>(args)...);

            sensors.insert(
                std::make_pair<AllSensors::key_type, AllSensors::mapped_type>(
                    std::weak_ptr<const interfaces::Sensor::Id>(
                        std::static_pointer_cast<const interfaces::Sensor::Id>(
                            sensorId)),
                    std::weak_ptr<interfaces::Sensor>(
                        std::static_pointer_cast<interfaces::Sensor>(sensor))));

            return std::static_pointer_cast<interfaces::Sensor>(sensor);
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
            if (it->first.expired())
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
                                const core::interfaces::Sensor::Id& s)
{
    return o << s.str();
}

inline std::ostream& operator<<(std::ostream& o,
                                const core::interfaces::Sensor& s)
{
    return o << s.id();
}

template <class S>
inline typename std::enable_if<
    std::is_base_of<core::interfaces::Sensor::Id, S>::value,
    std::ostream&>::type
    operator<<(std::ostream& o, const std::shared_ptr<const S>& s)
{
    return details::streamPtr<S>(o, s);
}

template <class S>
inline
    typename std::enable_if<std::is_base_of<core::interfaces::Sensor, S>::value,
                            std::ostream&>::type
    operator<<(std::ostream& o, const std::shared_ptr<const S>& s)
{
    return details::streamPtr<S>(o, s);
}
