#pragma once

#include "interfaces/sensor.hpp"
#include "interfaces/sensor_listener.hpp"
#include "interfaces/threshold.hpp"
#include "interfaces/trigger_action.hpp"
#include "interfaces/types.hpp"

#include <boost/asio/steady_timer.hpp>

#include <chrono>
#include <memory>
#include <vector>

class NumericThreshold :
    public interfaces::Threshold,
    public interfaces::SensorListener,
    public std::enable_shared_from_this<NumericThreshold>
{
  public:
    NumericThreshold(
        boost::asio::io_context& ioc,
        std::vector<std::shared_ptr<interfaces::Sensor>> sensors,
        std::vector<std::unique_ptr<interfaces::TriggerAction>> actions,
        NumericThresholdType thresholdType, std::chrono::milliseconds dwellTime,
        NumericActivationType activationType, double thresholdValue);
    ~NumericThreshold();

    void initialize();
    void sensorUpdated(interfaces::Sensor&, uint64_t) override;
    void sensorUpdated(interfaces::Sensor&, uint64_t, double value) override;

  private:
    boost::asio::steady_timer timer;
    const std::vector<std::shared_ptr<interfaces::Sensor>> sensors;
    const std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;
    const NumericThresholdType thresholdType;
    const std::chrono::milliseconds dwellTime;
    const NumericActivationType activationType;
    const double thresholdValue;

    struct ThresholdDetail
    {
        double prevValue;
        bool dwell;
    };
    std::vector<ThresholdDetail> details;

    void log(uint64_t, double);
    void startTimer(uint64_t, double, bool&);
    ThresholdDetail& getDetails(interfaces::Sensor& sensor);
};
