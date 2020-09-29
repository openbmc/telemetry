#pragma once

#include "interfaces/sensor.hpp"
#include "interfaces/sensor_listener.hpp"

#include <sdbusplus/asio/connection.hpp>

#include <memory>

class Sensor final :
    public interfaces::Sensor,
    public std::enable_shared_from_this<Sensor>
{
    using ValueVariant = std::variant<double, int64_t>;

  public:
    Sensor(interfaces::Sensor::Id sensorId,
           const std::shared_ptr<sdbusplus::asio::connection>& bus);

    Sensor(const Sensor&) = delete;
    Sensor& operator=(const Sensor&) = delete;

    virtual ~Sensor();

    static Id makeId(std::string_view service, std::string_view path);
    Id id() const override;
    void async_read() override;
    void registerForUpdates(
        const std::weak_ptr<interfaces::SensorListener>& weakListener) override;
    void schedule(std::chrono::milliseconds interval);

  private:
    interfaces::Sensor::Id sensorId;
    std::shared_ptr<sdbusplus::asio::connection> bus;

    std::vector<std::weak_ptr<interfaces::SensorListener>> listeners;
    std::optional<double> value;

    static std::optional<double> readValue(const ValueVariant& v);
};
