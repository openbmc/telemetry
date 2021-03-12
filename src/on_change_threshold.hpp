#pragma once

#include "interfaces/sensor.hpp"
#include "interfaces/sensor_listener.hpp"
#include "interfaces/threshold.hpp"
#include "interfaces/trigger_action.hpp"
#include "interfaces/trigger_types.hpp"

#include <boost/asio/steady_timer.hpp>

#include <chrono>
#include <memory>
#include <vector>

class OnChangeThreshold :
    public interfaces::Threshold,
    public interfaces::SensorListener,
    public std::enable_shared_from_this<OnChangeThreshold>
{
  public:
    OnChangeThreshold(
        std::vector<std::shared_ptr<interfaces::Sensor>> sensors,
        std::vector<std::string> sensorNames,
        std::vector<std::unique_ptr<interfaces::TriggerAction>> actions);
    ~OnChangeThreshold()
    {}

    void initialize() override;
    void sensorUpdated(interfaces::Sensor&, uint64_t) override;
    void sensorUpdated(interfaces::Sensor&, uint64_t, double) override;

  private:
    const std::vector<std::shared_ptr<interfaces::Sensor>> sensors;
    const std::vector<std::string> sensorNames;
    const std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;

    void commit(const std::string&, uint64_t, double);
};
