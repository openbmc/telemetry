#pragma once

#include "interfaces/sensor.hpp"
#include "interfaces/sensor_listener.hpp"
#include "types/milliseconds.hpp"
#include "utils/unique_call.hpp"

#include <boost/asio/high_resolution_timer.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/bus/match.hpp>

#include <memory>

class Sensor final :
    public interfaces::Sensor,
    public std::enable_shared_from_this<Sensor>
{
    using ValueVariant = std::variant<std::monostate, double>;

  public:
    Sensor(interfaces::Sensor::Id sensorId, const std::string& sensorMetadata,
           boost::asio::io_context& ioc,
           const std::shared_ptr<sdbusplus::asio::connection>& bus);

    Sensor(const Sensor&) = delete;
    Sensor& operator=(const Sensor&) = delete;

    static Id makeId(std::string_view service, std::string_view path);

    Id id() const override;
    std::string metadata() const override;
    void registerForUpdates(
        const std::weak_ptr<interfaces::SensorListener>& weakListener) override;
    void unregisterFromUpdates(
        const std::weak_ptr<interfaces::SensorListener>& weakListener) override;

  private:
    static std::optional<double> readValue(const ValueVariant& v);
    static void signalProc(const std::weak_ptr<Sensor>& weakSelf,
                           sdbusplus::message::message&);

    void async_read();
    void async_read(std::shared_ptr<utils::UniqueCall::Lock>);
    void makeSignalMonitor();
    void updateValue(double);

    interfaces::Sensor::Id sensorId;
    std::string sensorMetadata;
    boost::asio::io_context& ioc;
    std::shared_ptr<sdbusplus::asio::connection> bus;
    Milliseconds timerInterval = Milliseconds(0);
    std::optional<boost::asio::high_resolution_timer> timer;

    utils::UniqueCall uniqueCall;
    std::vector<std::weak_ptr<interfaces::SensorListener>> listeners;
    uint64_t timestamp = 0;
    std::optional<double> value;
    std::unique_ptr<sdbusplus::bus::match_t> signalMonitor;
};
