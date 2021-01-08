#pragma once

#include "interfaces/report_manager.hpp"
#include "interfaces/sensor.hpp"
#include "interfaces/trigger_factory.hpp"
#include "sensor_cache.hpp"

#include <sdbusplus/asio/object_server.hpp>

#include <memory>

class TriggerFactory : public interfaces::TriggerFactory
{
  public:
    TriggerFactory(std::shared_ptr<sdbusplus::asio::connection> bus,
                   std::shared_ptr<sdbusplus::asio::object_server> objServer,
                   SensorCache& sensorCache,
                   interfaces::ReportManager& reportManager);

    std::unique_ptr<interfaces::Trigger>
        make(boost::asio::yield_context& yield, const std::string& name,
             const bool isDiscrete, const bool logToJournal,
             const bool logToRedfish, const bool updateReport,
             const std::vector<sdbusplus::message::object_path>& sensorPaths,
             const std::vector<std::string>& reportNames,
             const TriggerThresholdParams& thresholdParams,
             interfaces::TriggerManager& triggerManager) const override;

  private:
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    SensorCache& sensorCache;
    interfaces::ReportManager& reportManager;

    std::vector<std::shared_ptr<interfaces::Sensor>> getSensors(
        boost::asio::yield_context& yield,
        const std::vector<sdbusplus::message::object_path>& sensorPaths) const;
};
