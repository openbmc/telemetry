#pragma once

#include "interfaces/sensor.hpp"
#include "interfaces/sensor_listener.hpp"
#include "utils/unique_call.hpp"

#include <boost/asio/high_resolution_timer.hpp>
#include <sdbusplus/asio/connection.hpp>

#include <memory>

class Sensor final :
    public interfaces::Sensor,
    public std::enable_shared_from_this<Sensor>
{
    using ValueVariant = std::variant<double, int64_t>;

  public:
    Sensor(interfaces::Sensor::Id sensorId, boost::asio::io_context& ioc,
           const std::shared_ptr<sdbusplus::asio::connection>& bus);

    Sensor(const Sensor&) = delete;
    Sensor& operator=(const Sensor&) = delete;

    virtual ~Sensor();

    static Id makeId(std::string_view service, std::string_view path);

    Id id() const override;
    void async_read() override;
    void registerForUpdates(
        const std::weak_ptr<interfaces::SensorListener>& weakListener) override;
    void schedule(std::chrono::milliseconds interval) override;
    void stop() override;

  private:
    interfaces::Sensor::Id sensorId;
    boost::asio::io_context& ioc;
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::chrono::milliseconds timerInterval = std::chrono::milliseconds(0);
    std::optional<boost::asio::high_resolution_timer> timer;

    std::vector<std::weak_ptr<interfaces::SensorListener>> listeners;
    std::optional<double> value;

    static std::optional<double> readValue(const ValueVariant& v);
    static void timerProc(boost::system::error_code,
                          const std::weak_ptr<Sensor>& weakSelf);

    void async_read(std::unique_ptr<UniqueCall>);
};
