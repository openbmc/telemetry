#pragma once

#include "interfaces/report_manager.hpp"
#include "interfaces/sensor.hpp"
#include "interfaces/trigger_factory.hpp"
#include "sensor_cache.hpp"

#include <sdbusplus/asio/object_server.hpp>

class TriggerFactory : public interfaces::TriggerFactory
{
  public:
    TriggerFactory(std::shared_ptr<sdbusplus::asio::connection> bus,
                   std::shared_ptr<sdbusplus::asio::object_server> objServer,
                   SensorCache& sensorCache,
                   interfaces::ReportManager& reportManager);

    std::unique_ptr<interfaces::Trigger>
        make(const std::string& name,
             const std::vector<std::string>& triggerActions,
             const std::vector<std::string>& reportNames,
             interfaces::TriggerManager& triggerManager,
             interfaces::JsonStorage& triggerStorage,
             const LabeledTriggerThresholdParams& labeledThresholdParams,
             const std::vector<LabeledSensorInfo>& labeledSensorsinfo)
            const override;

    std::vector<LabeledSensorInfo>
        getLabeledSensorsInfo(boost::asio::yield_context& yield,
                              const SensorsInfo& sensorsInfo) const;

  private:
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    SensorCache& sensorCache;
    interfaces::ReportManager& reportManager;

    std::pair<std::vector<std::shared_ptr<interfaces::Sensor>>,
              std::vector<std::string>>
        getSensors(
            const std::vector<LabeledSensorInfo>& labeledSensorsInfo) const;
};
