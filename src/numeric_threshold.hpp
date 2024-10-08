#pragma once

#include "interfaces/clock.hpp"
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

class NumericThreshold :
    public interfaces::Threshold,
    public interfaces::SensorListener,
    public std::enable_shared_from_this<NumericThreshold>
{
  public:
    NumericThreshold(
        boost::asio::io_context& ioc, const std::string& triggerId,
        Sensors sensors,
        std::vector<std::unique_ptr<interfaces::TriggerAction>> actions,
        Milliseconds dwellTime, numeric::Direction direction,
        double thresholdValue, numeric::Type type,
        std::unique_ptr<interfaces::Clock> clock);
    NumericThreshold(const NumericThreshold&) = delete;
    NumericThreshold(NumericThreshold&&) = delete;

    void initialize() override;
    void sensorUpdated(interfaces::Sensor&, Milliseconds, double) override;
    LabeledThresholdParam getThresholdParam() const override;
    void updateSensors(Sensors newSensors) override;

  private:
    boost::asio::io_context& ioc;
    const std::string& triggerId;
    const std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;
    const Milliseconds dwellTime;
    const numeric::Direction direction;
    const double thresholdValue;
    const numeric::Type type;
    bool initialized = false;
    std::unique_ptr<interfaces::Clock> clock;

    struct ThresholdDetail
    {
        std::optional<double> prevValue = std::nullopt;
        numeric::Direction prevDirection = numeric::Direction::either;
        bool dwell = false;
        boost::asio::steady_timer timer;

        ThresholdDetail(const std::string& sensorNameIn,
                        boost::asio::io_context& ioc) :
            timer(ioc), sensorName(sensorNameIn)
        {}
        ThresholdDetail(const ThresholdDetail&) = delete;
        ThresholdDetail(ThresholdDetail&&) = delete;

        const std::string& getSensorName()
        {
            return sensorName;
        }

      private:
        std::string sensorName;
    };
    using SensorDetails =
        std::unordered_map<std::shared_ptr<interfaces::Sensor>,
                           std::shared_ptr<ThresholdDetail>>;
    SensorDetails sensorDetails;

    friend ThresholdOperations;

    void startTimer(ThresholdDetail&, double);
    void commit(const std::string&, double);
    ThresholdDetail& getDetails(const interfaces::Sensor& sensor);
    std::shared_ptr<ThresholdDetail> makeDetails(const std::string& sensorName);
};
