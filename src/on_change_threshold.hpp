#pragma once

#include "interfaces/sensor.hpp"
#include "interfaces/sensor_listener.hpp"
#include "interfaces/threshold.hpp"
#include "interfaces/trigger_action.hpp"
#include "types/trigger_types.hpp"

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
        Sensors sensors,
        std::vector<std::unique_ptr<interfaces::TriggerAction>> actions);
    ~OnChangeThreshold()
    {}

    void initialize() override;
    void sensorUpdated(interfaces::Sensor&, Milliseconds, double) override;
    LabeledThresholdParam getThresholdParam() const override;
    void updateSensors(Sensors newSensors) override;

  private:
    Sensors sensors;
    const std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;
    bool initialized = false;

    void commit(const std::string&, Milliseconds, double);
};
