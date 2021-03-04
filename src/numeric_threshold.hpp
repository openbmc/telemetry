#pragma once

#include "interfaces/sensor.hpp"
#include "interfaces/sensor_listener.hpp"
#include "interfaces/threshold.hpp"
#include "interfaces/trigger_action.hpp"
#include "interfaces/trigger_types.hpp"
#include "types/duration_type.hpp"

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
        std::vector<std::string> sensorNames,
        std::vector<std::unique_ptr<interfaces::TriggerAction>> actions,
        DurationType dwellTime, numeric::Direction direction,
        double thresholdValue);
    ~NumericThreshold();

    void initialize() override;
    void sensorUpdated(interfaces::Sensor&, uint64_t) override;
    void sensorUpdated(interfaces::Sensor&, uint64_t, double) override;

  private:
    boost::asio::io_context& ioc;
    const std::vector<std::shared_ptr<interfaces::Sensor>> sensors;
    const std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;
    const DurationType dwellTime;
    const numeric::Direction direction;
    const double thresholdValue;

    struct ThresholdDetail
    {
        std::string sensorName;
        double prevValue;
        bool dwell;
        boost::asio::steady_timer timer;

        ThresholdDetail(const std::string& name, double prevValue, bool dwell,
                        boost::asio::io_context& ioc) :
            sensorName(name),
            prevValue(prevValue), dwell(dwell), timer(ioc)
        {}
    };
    std::vector<ThresholdDetail> details;

    void startTimer(const std::string&, uint64_t, double, bool&,
                    boost::asio::steady_timer&);
    void commit(const std::string&, uint64_t, double);
    ThresholdDetail& getDetails(interfaces::Sensor& sensor);
};
