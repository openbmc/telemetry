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
        numeric::Level level, std::chrono::milliseconds dwellTime,
        numeric::Direction direction, double thresholdValue);
    ~NumericThreshold();

    void initialize();
    void sensorUpdated(interfaces::Sensor&, uint64_t) override;
    void sensorUpdated(interfaces::Sensor&, uint64_t, double value) override;

  private:
    boost::asio::io_context& ioc;
    const std::vector<std::shared_ptr<interfaces::Sensor>> sensors;
    const std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;
    const numeric::Level level;
    const std::chrono::milliseconds dwellTime;
    const numeric::Direction direction;
    const double thresholdValue;

    struct ThresholdDetail
    {
        double prevValue;
        bool dwell;
        boost::asio::steady_timer timer;

        ThresholdDetail(double prevValue, bool dwell,
                        boost::asio::io_context& ioc) :
            prevValue(prevValue),
            dwell(dwell), timer(ioc)
        {}
    };
    std::vector<ThresholdDetail> details;

    void startTimer(uint64_t, double, bool&, boost::asio::steady_timer&);
    void commit(uint64_t, double);
    ThresholdDetail& getDetails(interfaces::Sensor& sensor);
};
