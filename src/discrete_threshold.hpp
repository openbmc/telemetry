#pragma once

#include "interfaces/sensor.hpp"
#include "interfaces/sensor_listener.hpp"
#include "interfaces/threshold.hpp"
#include "interfaces/trigger_action.hpp"
#include "types/duration_types.hpp"
#include "types/trigger_types.hpp"
#include "utils/threshold_operations.hpp"

#include <boost/asio/steady_timer.hpp>

#include <chrono>
#include <map>
#include <memory>
#include <vector>

class DiscreteThreshold :
    public interfaces::Threshold,
    public interfaces::SensorListener,
    public std::enable_shared_from_this<DiscreteThreshold>
{
  public:
    DiscreteThreshold(
        boost::asio::io_context& ioc, Sensors sensors,
        std::vector<std::unique_ptr<interfaces::TriggerAction>> actions,
        Milliseconds dwellTime, double thresholdValue, const std::string& name,
        const discrete::Severity severity);
    DiscreteThreshold(const DiscreteThreshold&) = delete;
    DiscreteThreshold(DiscreteThreshold&&) = delete;

    void initialize() override;
    void sensorUpdated(interfaces::Sensor&, Milliseconds, double) override;
    LabeledThresholdParam getThresholdParam() const override;
    void updateSensors(Sensors newSensors) override;

  private:
    boost::asio::io_context& ioc;
    const std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;
    const Milliseconds dwellTime;
    const double thresholdValue;
    const std::string name;
    const discrete::Severity severity;
    bool initialized = false;

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
    using SensorDetails =
        std::unordered_map<std::shared_ptr<interfaces::Sensor>,
                           std::shared_ptr<ThresholdDetail>>;
    SensorDetails sensorDetails;

    friend ThresholdOperations;

    void startTimer(ThresholdDetail&, Milliseconds, double);
    void commit(const std::string&, Milliseconds, double);
    ThresholdDetail& getDetails(const interfaces::Sensor& sensor);
    std::shared_ptr<ThresholdDetail> makeDetails(const std::string& sensorName);
};
