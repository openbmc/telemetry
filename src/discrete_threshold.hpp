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

class DiscreteThreshold :
    public interfaces::Threshold,
    public interfaces::SensorListener,
    public std::enable_shared_from_this<DiscreteThreshold>
{
  public:
    DiscreteThreshold(
        boost::asio::io_context& ioc,
        std::vector<std::shared_ptr<interfaces::Sensor>> sensors,
        std::vector<std::string> sensorNames,
        std::vector<std::unique_ptr<interfaces::TriggerAction>> actions,
        std::chrono::milliseconds dwellTime, double thresholdValue,
        std::string name);
    ~DiscreteThreshold();

    void initialize() override;
    void sensorUpdated(interfaces::Sensor&, uint64_t) override;
    void sensorUpdated(interfaces::Sensor&, uint64_t, double) override;
    const char* getName() const;

  private:
    boost::asio::io_context& ioc;
    const std::vector<std::shared_ptr<interfaces::Sensor>> sensors;
    const std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;
    const std::chrono::milliseconds dwellTime;
    const double thresholdValue;
    const std::string name;

    struct ThresholdDetail
    {
        std::string sensorName;
        bool dwell;
        boost::asio::steady_timer timer;

        ThresholdDetail(const std::string& name, bool dwell,
                        boost::asio::io_context& ioc) :
            sensorName(name),
            dwell(dwell), timer(ioc)
        {}
    };
    std::vector<ThresholdDetail> details;

    void startTimer(const std::string&, uint64_t, double, bool&,
                    boost::asio::steady_timer&);
    void commit(const std::string&, uint64_t, double);
    ThresholdDetail& getDetails(interfaces::Sensor& sensor);
};
